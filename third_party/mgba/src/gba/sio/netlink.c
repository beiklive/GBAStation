/* SPDX-License-Identifier: MPL-2.0 */
#include <mgba/internal/gba/sio/netlink.h>

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

// Switch release: keep protocol errors in lastError, silence diagnostic logs.
#define NETLINK_DEBUG_LOG(...) ((void) 0)

#define NETLINK_PROTOCOL 7
#define NETLINK_ACTIVE_POLL_CYCLES 64
#define NETLINK_HANDSHAKE_POLL_CYCLES 1024
#define NETLINK_IDLE_POLL_CYCLES (GBA_ARM7TDMI_FREQUENCY / (60 * 64))
#define NETLINK_CONNECT_RETRY_CYCLES (GBA_ARM7TDMI_FREQUENCY / 2)

enum NetlinkFrameType {
	NETLINK_HELLO = 1,
	NETLINK_WELCOME,
	NETLINK_READY_FRAME,
	NETLINK_START,
	NETLINK_DATA,
	NETLINK_GP,
	NETLINK_CLOSE,
	NETLINK_MODE,
	NETLINK_CLOCK,
};

static bool _init(struct GBASIODriver*);
static void _deinit(struct GBASIODriver*);
static bool _load(struct GBASIODriver*);
static bool _unload(struct GBASIODriver*);
static uint16_t _write(struct GBASIODriver*, uint32_t, uint16_t);
static bool _rcntInit(struct GBASIODriver*);
static void _rcntDeinit(struct GBASIODriver*);
static uint16_t _rcntWrite(struct GBASIODriver*, uint32_t, uint16_t);
static void _process(struct mTiming*, void*, uint32_t);
static void _handle(struct GBASIONetlink*);
static void _flush(struct GBASIONetlink*);
static void _advanceLinkTime(struct GBASIONetlink*, struct mTiming*);
static const char* _connectionName(enum GBASIONetlinkConnectionState);
static void _traceText(struct GBASIONetlink*, const char*, const char*);
static void _performanceReport(struct GBASIONetlink*, const char*, bool);

static void _put16(uint8_t* p, uint16_t value) {
	p[0] = value >> 8;
	p[1] = value;
}

static void _put32(uint8_t* p, uint32_t value) {
	p[0] = value >> 24;
	p[1] = value >> 16;
	p[2] = value >> 8;
	p[3] = value;
}

static uint16_t _get16(const uint8_t* p) {
	return ((uint16_t) p[0] << 8) | p[1];
}

static uint32_t _get32(const uint8_t* p) {
	return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | p[3];
}

static uint64_t _get64(const uint8_t* p) {
 return ((uint64_t) _get32(p) << 32) | _get32(p + 4);
}
static void _put64(uint8_t* p, uint64_t value) {
 _put32(p, value >> 32); _put32(p + 4, value);
}

static enum GBASIONetlinkConnectionState _connectionState(const struct GBASIONetlink* net) {
	int state;
	ATOMIC_LOAD(state, net->connectionState);
	return state;
}

static void _setConnectionState(struct GBASIONetlink* net, enum GBASIONetlinkConnectionState state) {
	enum GBASIONetlinkConnectionState previous = _connectionState(net);
	ATOMIC_STORE(net->connectionState, state);
	if (previous != state) {
		char detail[96];
		snprintf(detail, sizeof(detail), "\"from\":\"%s\",\"to\":\"%s\"", _connectionName(previous), _connectionName(state));
		_traceText(net, "connection_state", detail);
		_performanceReport(net, "state", true);
	}
}

static const char* _roleName(const struct GBASIONetlink* net) {
	return net->role == GBA_NETLINK_HOST ? "host" : "join";
}

static const char* _frameName(enum NetlinkFrameType type) {
	switch (type) {
	case NETLINK_CLOCK: return "clock";
	case NETLINK_HELLO: return "hello";
	case NETLINK_WELCOME: return "welcome";
	case NETLINK_READY_FRAME: return "ready";
	case NETLINK_START: return "start";
	case NETLINK_DATA: return "data";
	case NETLINK_GP: return "gp";
	case NETLINK_CLOSE: return "close";
	case NETLINK_MODE: return "mode";
	default: return "unknown";
	}
}

static const char* _connectionName(enum GBASIONetlinkConnectionState state) {
	switch (state) {
	case GBA_NETLINK_DISCONNECTED: return "disconnected";
	case GBA_NETLINK_LISTENING: return "listening";
	case GBA_NETLINK_CONNECTING: return "connecting";
	case GBA_NETLINK_HANDSHAKE: return "handshake";
	case GBA_NETLINK_READY: return "ready";
	case GBA_NETLINK_ERROR: return "error";
	default: return "unknown";
	}
}

static const char* _transferName(enum GBASIONetlinkTransferState state) {
	switch (state) {
	case GBA_NETLINK_IDLE: return "idle";
	case GBA_NETLINK_HOST_WAIT_PEER: return "host_wait_peer";
	case GBA_NETLINK_JOIN_WAIT_CLOCK: return "join_wait_clock";
	case GBA_NETLINK_TRANSFER: return "transfer";
	default: return "unknown";
	}
}

// Test-only wall-time telemetry, independent of the high-volume protocol trace.
static void _performanceReport(struct GBASIONetlink* net, const char* event, bool force) {
	if (!net->performanceFile || !net->performanceClockUs) return;
	uint64_t now = net->performanceClockUs();
	uint64_t window = now - net->performanceReportUs;
	if (!force && window < 1000000) return;
	uint64_t wait = net->performanceHostWaitUs + net->performanceJoinWaitUs;
	fprintf(net->performanceFile,
		"%llu,%s,%s,%s,%u,%d,%04X,%d,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%llu,%llu,%.3f,%.3f,%d,\"%s\",%llu,%.3f,%.3f,%llu,%.3f,%.3f,event_clock_v7,%llu,%llu,%llu,%llu\n",
		(unsigned long long) now, event, _roleName(net), _connectionName(_connectionState(net)),
		net->transferSequence, net->d.p ? (int) net->d.p->mode : -1, net->d.p ? net->d.p->siocnt : 0,
		net->sessionLive, net->localMultiActive, net->peerMultiActive, net->transferActive,
		net->hasPendingStart, net->linkTime,
		window / 1000.0,
		window ? (wait - net->performanceReportedWaitUs) * 100.0 / window : 0.0,
		window ? (net->performanceFrames - net->performanceReportedFrames) * 1000000.0 / window : 0.0,
		net->performanceMaxFrameGapUs / 1000.0,
		net->performanceHostWaitUs / 1000.0, net->performanceJoinWaitUs / 1000.0,
		(unsigned long long) net->performanceWaitCalls, (unsigned long long) net->performanceRoundCount,
		net->performanceRoundCount ? net->performanceRoundTotalUs / (1000.0 * net->performanceRoundCount) : 0.0,
		net->performanceRoundMaxUs / 1000.0, net->performanceSocketError, net->lastError,
		(unsigned long long) net->performanceStartSendCount,
		net->performanceStartSendCount ? net->performanceStartSendTotalUs / (1000.0 * net->performanceStartSendCount) : 0.0,
		net->performanceStartSendMaxUs / 1000.0,
		(unsigned long long) net->performanceSocketDataCount,
		net->performanceSocketDataCount ? net->performanceSocketDataTotalUs / (1000.0 * net->performanceSocketDataCount) : 0.0,
		net->performanceSocketDataMaxUs / 1000.0,
		(unsigned long long) net->syncClock, (unsigned long long) net->peerHorizon,
		(unsigned long long) net->transferStartClock, (unsigned long long) net->transferFinishClock);
	fflush(net->performanceFile);
	net->performanceReportUs = now;
	net->performanceReportedWaitUs = wait;
	net->performanceReportedFrames = net->performanceFrames;
	net->performanceMaxFrameGapUs = 0;
}

bool GBASIONetlinkSetPerformancePath(struct GBASIONetlink* net, const char* path, uint64_t (*clockUs)(void)) {
	if (!path || !clockUs || net->performanceFile) return false;
	net->performanceFile = fopen(path, "w");
	if (!net->performanceFile) return false;
	net->performanceClockUs = clockUs;
	net->performanceReportUs = clockUs();
	fputs("wall_us,event,role,connection,seq,mode,siocnt,session_live,local_multi,peer_multi,transfer_active,pending_start,link_cycles,window_ms,wait_pct,fps,frame_gap_max_ms,host_wait_ms_total,join_wait_ms_total,wait_calls_total,rounds_total,start_to_data_avg_ms,start_to_data_max_ms,socket_error,reason,start_sent_total,start_to_socket_avg_ms,start_to_socket_max_ms,data_received_total,socket_to_data_avg_ms,socket_to_data_max_ms,implementation,sync_clock,peer_horizon,transfer_start_clock,transfer_finish_clock\n", net->performanceFile);
	_performanceReport(net, "open", true);
	return true;
}

void GBASIONetlinkRecordFrame(struct GBASIONetlink* net) {
	if (!net || !net->performanceFile) return;
	uint64_t now = net->performanceClockUs();
	if (net->performanceLastFrameUs) {
		uint64_t gap = now - net->performanceLastFrameUs;
		if (gap > net->performanceMaxFrameGapUs) net->performanceMaxFrameGapUs = gap;
	}
	net->performanceLastFrameUs = now;
	++net->performanceFrames;
	_performanceReport(net, "summary", false);
}

static void _traceText(struct GBASIONetlink* net, const char* event, const char* detail) {
	if (!net->traceFile) return;
	fprintf(net->traceFile,
		"{\"trace_version\":1,\"order\":%llu,\"event\":\"%s\",\"role\":\"%s\","
		"\"connection\":\"%s\",\"transfer\":\"%s\",\"cycle\":%llu,\"link_time\":%d%s%s}\n",
		(unsigned long long) ++net->traceOrder, event, _roleName(net),
		_connectionName(_connectionState(net)), _transferName(net->transferState),
		(unsigned long long) net->lastCycle, net->linkTime,
		detail && detail[0] ? "," : "", detail && detail[0] ? detail : "");
	if (!(net->traceOrder & 4095) || !strcmp(event, "connection_state")
			|| !strcmp(event, "close") || !strcmp(event, "trace_end")) fflush(net->traceFile);
}

static void _traceFrame(struct GBASIONetlink* net, const char* direction, const uint8_t* frame) {
	if (!net->traceFile) return;
	char hex[GBA_NETLINK_FRAME_SIZE * 2 + 1];
	for (size_t i = 0; i < GBA_NETLINK_FRAME_SIZE; ++i) snprintf(&hex[i * 2], 3, "%02x", frame[i]);
	fprintf(net->traceFile,
		"{\"trace_version\":1,\"order\":%llu,\"event\":\"frame\",\"dir\":\"%s\","
		"\"frame_type\":\"%s\",\"role\":\"%s\",\"connection\":\"%s\","
		"\"transfer\":\"%s\",\"cycle\":%llu,\"link_time\":%d,\"session\":%u,"
		"\"seq\":%u,\"value\":%u,\"siocnt\":%u,\"frame\":\"%s\"}\n",
		(unsigned long long) ++net->traceOrder, direction, _frameName(frame[5]), _roleName(net),
		_connectionName(_connectionState(net)), _transferName(net->transferState),
		(unsigned long long) net->lastCycle, net->linkTime, _get32(&frame[8]), _get32(&frame[12]),
		_get16(&frame[16]), _get16(&frame[18]), hex);
	if (!(net->traceOrder & 4095)) fflush(net->traceFile);
}

static void _traceChunk(struct GBASIONetlink* net, const char* direction, size_t size, size_t offset) {
	char detail[96];
	snprintf(detail, sizeof(detail), "\"dir\":\"%s\",\"size\":%zu,\"offset\":%zu", direction, size, offset);
	_traceText(net, "tcp_chunk", detail);
}

static uint64_t _diagnosticCycle(const struct GBASIONetlink* net) {
	return net->d.p ? mTimingGlobalTime(&net->d.p->p->timing) : net->lastCycle;
}

static struct GBASIONetlinkDiagnostic* _diagnosticStart(struct GBASIONetlink* net,
		uint32_t sequence, uint16_t hostValue, uint16_t siocnt) {
	size_t index = net->diagnosticNext;
	struct GBASIONetlinkDiagnostic* record = &net->diagnostic[index];
	memset(record, 0, sizeof(*record));
	record->valid = true;
	record->session = net->session;
	record->sequence = sequence;
	record->hostValue = hostValue;
	record->lastSendWrite = net->diagnosticLastSend;
	record->sendWriteCycle = net->diagnosticLastSendCycle;
	record->startCycle = _diagnosticCycle(net);
	record->siocntStart = siocnt;
	record->rcntStart = net->d.p ? net->d.p->rcnt : 0;
	record->modeStart = net->d.p ? net->d.p->mode : 0xFF;
	net->diagnosticCurrent = index;
	net->diagnosticNext = (index + 1) % GBA_NETLINK_DIAGNOSTIC_CAPACITY;
	if (net->diagnosticCount < GBA_NETLINK_DIAGNOSTIC_CAPACITY) ++net->diagnosticCount;
	return record;
}

static struct GBASIONetlinkDiagnostic* _diagnosticCurrent(struct GBASIONetlink* net) {
	struct GBASIONetlinkDiagnostic* record = &net->diagnostic[net->diagnosticCurrent];
	if (!record->valid || record->session != net->session || record->sequence != net->transferSequence) return NULL;
	return record;
}

static const char* _stateDiagnosticName(uint8_t event) {
	switch (event) {
	case GBA_NETLINK_STATE_LOAD: return "multi_load";
	case GBA_NETLINK_STATE_UNLOAD: return "multi_unload";
	case GBA_NETLINK_STATE_GP_ENTER: return "gp_enter";
	case GBA_NETLINK_STATE_GP_LEAVE: return "gp_leave";
	case GBA_NETLINK_STATE_HOLD_ENTER: return "hold_enter";
	case GBA_NETLINK_STATE_HOLD_EXIT: return "hold_exit";
	case GBA_NETLINK_STATE_SIOCNT_WRITE: return "siocnt_write";
	case GBA_NETLINK_STATE_START_DISCARD: return "start_discard";
	case GBA_NETLINK_STATE_START_QUEUED: return "start_queued";
	case GBA_NETLINK_STATE_CLOSE: return "close";
	case GBA_NETLINK_STATE_PEER_MODE: return "peer_mode";
	default: return "unknown";
	}
}

static void _diagnosticState(struct GBASIONetlink* net, uint8_t event, uint16_t value) {
	struct GBASIONetlinkStateDiagnostic* record = &net->stateDiagnostic[net->stateDiagnosticNext];
	memset(record, 0, sizeof(*record));
	record->cycle = _diagnosticCycle(net);
	record->linkTime = net->linkTime;
	record->sequence = net->transferSequence;
	record->value = value;
	record->event = event;
	record->connection = _connectionState(net);
	record->transferActive = net->transferActive;
	record->sessionLive = net->sessionLive;
	record->pendingStart = net->hasPendingStart;
	record->localMultiActive = net->localMultiActive;
	record->peerMultiActive = net->peerMultiActive;
	if (net->d.p) {
		record->mode = net->d.p->mode;
		record->rcnt = net->d.p->rcnt;
		record->siocnt = net->d.p->siocnt;
	} else {
		record->mode = 0xFF;
	}
	net->stateDiagnosticNext = (net->stateDiagnosticNext + 1) % GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY;
	if (net->stateDiagnosticCount < GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY) {
		++net->stateDiagnosticCount;
	}
}

static void _diagnosticHold(struct GBASIONetlink* net, bool active) {
	if (net->stateDiagnosticHoldActive == active) return;
	net->stateDiagnosticHoldActive = active;
	_diagnosticState(net, active ? GBA_NETLINK_STATE_HOLD_ENTER : GBA_NETLINK_STATE_HOLD_EXIT, 0);
}

static void _diagnosticDump(struct GBASIONetlink* net) {
	if (!net->diagnosticPath[0]) return;
	if (net->diagnosticCount) {
		FILE* file = fopen(net->diagnosticPath, "w");
		if (file) {
			fputs("role,session,seq,last_send_write,host_value,latched_value,remote_value,send_write_cycle,start_cycle,latch_cycle,finish_cycle,mode_start,mode_latch,siocnt_start,siocnt_latch,rcnt_start,rcnt_latch,rcnt_finish\n", file);
			size_t first = (net->diagnosticNext + GBA_NETLINK_DIAGNOSTIC_CAPACITY - net->diagnosticCount)
				% GBA_NETLINK_DIAGNOSTIC_CAPACITY;
			for (size_t i = 0; i < net->diagnosticCount; ++i) {
				const struct GBASIONetlinkDiagnostic* record = &net->diagnostic[(first + i) % GBA_NETLINK_DIAGNOSTIC_CAPACITY];
				if (!record->valid) continue;
				fprintf(file, "%s,%08X,%u,%04X,%04X,%04X,%04X,%llu,%llu,%llu,%llu,%u,%u,%04X,%04X,%04X,%04X,%04X\n",
					_roleName(net), record->session, record->sequence, record->lastSendWrite, record->hostValue,
					record->latchedValue, record->remoteValue, (unsigned long long) record->sendWriteCycle,
					(unsigned long long) record->startCycle, (unsigned long long) record->latchCycle,
					(unsigned long long) record->finishCycle, record->modeStart, record->modeLatch,
					record->siocntStart, record->siocntLatch, record->rcntStart,
					record->rcntLatch, record->rcntFinish);
			}
			fclose(file);
		}
	}
	if (net->stateDiagnosticCount) {
		char path[sizeof(net->diagnosticPath) + 16];
		snprintf(path, sizeof(path), "%s.state.csv", net->diagnosticPath);
		FILE* file = fopen(path, "w");
		if (!file) return;
		fputs("role,event,cycle,link_time,seq,value,mode,connection,transfer_active,session_live,pending_start,local_multi,peer_multi,rcnt,siocnt\n", file);
		size_t first = (net->stateDiagnosticNext + GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY
			- net->stateDiagnosticCount) % GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY;
		for (size_t i = 0; i < net->stateDiagnosticCount; ++i) {
			const struct GBASIONetlinkStateDiagnostic* record =
				&net->stateDiagnostic[(first + i) % GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY];
			fprintf(file, "%s,%s,%llu,%d,%u,%04X,%u,%u,%u,%u,%u,%u,%u,%04X,%04X\n",
				_roleName(net), _stateDiagnosticName(record->event),
				(unsigned long long) record->cycle, record->linkTime, record->sequence,
				record->value, record->mode, record->connection,
				record->transferActive, record->sessionLive, record->pendingStart,
				record->localMultiActive, record->peerMultiActive,
				record->rcnt, record->siocnt);
		}
		fclose(file);
	}
}

static bool _isReady(const struct GBASIONetlink* net) {
	return _connectionState(net) == GBA_NETLINK_READY && !net->closing;
}

static bool _isCableReady(const struct GBASIONetlink* net) {
	return _isReady(net) && net->localMultiActive && net->peerMultiActive;
}

static int32_t _transferCyclesFor(uint16_t siocnt) {
	return GBASIOCyclesPerTransfer[GBASIOMultiplayerGetBaud(siocnt)][1];
}

static void _normaliseClock(struct GBASIONetlink* net) {
	if (net->linkTime == INT32_MAX) {
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK CLOCK_RESET role=%s", _roleName(net));
		net->linkTime = 0;
	}
}

static void _advanceLinkTime(struct GBASIONetlink* net, struct mTiming* timing) {
	uint64_t now = mTimingGlobalTime(timing);
	if (!net->lastCycle) {
		net->lastCycle = now;
		return;
	}
	uint64_t elapsed = now - net->lastCycle;
	// A held core advances only the one-cycle polling callbacks. Do not
	// grant those waits as peer-authorized emulation time.
	if (!net->clockHeld) net->syncClock += elapsed;
	if (elapsed > INT32_MAX || net->linkTime > INT32_MAX - (int32_t) elapsed) {
		net->linkTime = INT32_MAX;
	} else {
		net->linkTime += elapsed;
	}
	net->lastCycle = now;
}

static void _setError(struct GBASIONetlink* net, const char* reason) {
	strncpy(net->lastError, reason, sizeof(net->lastError) - 1);
	net->lastError[sizeof(net->lastError) - 1] = '\0';
	_performanceReport(net, "error", true);
}

static void _close(struct GBASIONetlink* net, const char* reason) {
	if (!net->closing) {
		NETLINK_DEBUG_LOG(GBA_SIO, WARN, "NETLINK CLOSE role=%s reason=%s", _roleName(net), reason);
		_setError(net, reason);
		_diagnosticState(net, GBA_NETLINK_STATE_CLOSE, 0);
	}
	net->closing = true;
	char detail[160];
	snprintf(detail, sizeof(detail), "\"reason\":\"%s\"", reason);
	_traceText(net, "close", detail);
}

static void _makeFrame(struct GBASIONetlink* net, uint8_t* frame, enum NetlinkFrameType type,
		uint32_t sequence, uint16_t value, uint16_t siocnt) {
	memset(frame, 0, GBA_NETLINK_FRAME_SIZE);
	memcpy(frame, "MGNL", 4);
	frame[4] = NETLINK_PROTOCOL;
	frame[5] = type;
	frame[6] = net->role;
	frame[7] = 2;
	_put32(&frame[8], type == NETLINK_HELLO ? 0 : net->session);
	_put32(&frame[12], sequence);
	_put16(&frame[16], value);
	_put16(&frame[18], siocnt);
	_put32(&frame[20], type == NETLINK_START ? (uint32_t) net->peerStartTime
		: type == NETLINK_WELCOME ? (uint32_t) net->linkTime : 0);
	_put16(&frame[24], net->received[0]);
	_put16(&frame[26], net->received[1]);
	if (type == NETLINK_START || type == NETLINK_CLOCK || type == NETLINK_WELCOME
			|| type == NETLINK_MODE || type == NETLINK_GP)
		_put64(&frame[20], net->syncClock);
}

static bool _queue(struct GBASIONetlink* net, enum NetlinkFrameType type,
		uint32_t sequence, uint16_t value, uint16_t siocnt) {
	if (net->txCount == GBA_NETLINK_TX_CAPACITY) {
		_close(net, "outbound queue full");
		return false;
	}
	size_t tail = (net->txHead + net->txCount) % GBA_NETLINK_TX_CAPACITY;
	_makeFrame(net, net->tx[tail].data, type, sequence, value, siocnt);
	_traceFrame(net, "tx", net->tx[tail].data);
	net->tx[tail].offset = 0;
	++net->txCount;
	// Send on the same emulation-thread turn that produced the message. The
	// socket is nonblocking; partial writes retain their offset and FIFO order
	// for the normal transport callback to retry. Do not run receive/finish here.
	if (!net->connectingSocket) _flush(net);
	return true;
}

static void _publishMultiMode(struct GBASIONetlink* net) {
	if (net->d.p) _advanceLinkTime(net, &net->d.p->p->timing);
	if (!_isReady(net)) {
		return;
	}
	uint16_t siocnt = net->d.p ? net->d.p->siocnt : 0;
	_queue(net, NETLINK_MODE, net->transferSequence, net->localMultiActive ? 1 : 0, siocnt);
	NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK MODE_SEND role=%s active=%i seq=%u",
		_roleName(net), net->localMultiActive, net->transferSequence);
}

static void _flush(struct GBASIONetlink* net) {
	while (!SOCKET_FAILED(net->socket) && net->txCount && !net->closing) {
		struct GBASIONetlinkTxFrame* frame = &net->tx[net->txHead];
		ssize_t sent = SocketSend(net->socket, frame->data + frame->offset,
			GBA_NETLINK_FRAME_SIZE - frame->offset);
		if (sent > 0) {
			size_t remaining = GBA_NETLINK_FRAME_SIZE - frame->offset;
			if ((size_t) sent < remaining) _traceChunk(net, "tx", sent, frame->offset);
			frame->offset += sent;
			if (frame->offset == GBA_NETLINK_FRAME_SIZE) {
				// This is acceptance by the local socket, not a wire/peer timestamp.
				if (net->performanceFile && frame->data[5] == NETLINK_START
						&& _get32(&frame->data[12]) == net->transferSequence
						&& net->performanceRoundStartUs) {
					net->performanceStartSentUs = net->performanceClockUs();
					uint64_t elapsed = net->performanceStartSentUs - net->performanceRoundStartUs;
					++net->performanceStartSendCount;
					net->performanceStartSendTotalUs += elapsed;
					if (elapsed > net->performanceStartSendMaxUs) net->performanceStartSendMaxUs = elapsed;
				}
				frame->offset = 0;
				net->txHead = (net->txHead + 1) % GBA_NETLINK_TX_CAPACITY;
				--net->txCount;
			}
			continue;
		}
		if (!sent) {
			_close(net, "send returned zero");
		} else if (!SocketWouldBlock()) {
			net->performanceSocketError = SocketError();
			_close(net, "send failed");
		}
		break;
	}
}

static void _sendCloseBestEffort(struct GBASIONetlink* net) {
	if (SOCKET_FAILED(net->socket) || !net->session) {
		return;
	}
	uint8_t frame[GBA_NETLINK_FRAME_SIZE];
	_makeFrame(net, frame, NETLINK_CLOSE, net->transferSequence, 0, 0);
	_traceFrame(net, "tx", frame);
	SocketSend(net->socket, frame, sizeof(frame));
}

static void _setCableRegisters(struct GBASIONetlink* net, bool ready) {
	if (!net->d.p) {
		return;
	}
	struct GBASIO* sio = net->d.p;
	if (sio->mode != SIO_MULTI) {
		return;
	}
	// Model the cable line phases: SC is low during a transfer and high
	// while idle; SD identifies the slave. These four
	// RCNT data bits are hardware status in multiplayer mode.
	uint16_t pins;
	if (net->role == GBA_NETLINK_JOIN) {
		pins = net->transferActive ? 6 : 7;
	} else {
		pins = net->transferActive ? 2 : 3;
	}
	sio->rcnt = (sio->rcnt & ~0xF) | pins;
	if (net->role == GBA_NETLINK_JOIN) {
		sio->siocnt = GBASIOMultiplayerFillSlave(sio->siocnt);
	} else {
		sio->siocnt = GBASIOMultiplayerClearSlave(sio->siocnt);
	}
	sio->siocnt = GBASIOMultiplayerSetReady(sio->siocnt, ready);
	if (ready) {
		sio->siocnt = GBASIOMultiplayerClearError(sio->siocnt);
	}
}

static void _cancelTransfer(struct GBASIONetlink* net, bool error) {
	if (net->hasPendingStart) {
		uint32_t pendingSequence = _get32(&net->pendingStart[12]);
		if (pendingSequence > net->transferSequence) {
			net->transferSequence = pendingSequence;
		}
	}
	net->transferActive = false;
	net->remoteDataReady = false;
	net->localDataSent = false;
	net->hasPendingStart = false;
	net->transferState = GBA_NETLINK_IDLE;
	if (net->d.p) {
		net->d.p->siocnt = GBASIOMultiplayerClearBusy(net->d.p->siocnt);
		_setCableRegisters(net, _isCableReady(net));
		if (error) {
			net->d.p->siocnt = GBASIOMultiplayerFillError(net->d.p->siocnt);
		}
	}
}

static void _resetCableEpoch(struct GBASIONetlink* net) {
	_cancelTransfer(net, false);
	net->sessionLive = false;
	net->linkTime = 0;
	net->peerStartTime = 0;
	net->transferCycles = 0;
	net->aheadHoldAfterCycle = 0;
	net->received[0] = 0xFFFF;
	net->received[1] = 0xFFFF;
	net->received[2] = 0xFFFF;
	net->received[3] = 0xFFFF;
	_diagnosticHold(net, false);
}

static void _resetPeerSession(struct GBASIONetlink* net) {
	net->peerMultiActive = false;
	_cancelTransfer(net, false);
	net->session = 0;
	net->sequence = 0;
	net->transferSequence = 0;
	net->linkTime = 0;
	net->lastCycle = 0;
	net->syncClock = net->peerHorizon = net->publishedClock = 0;
	net->clockHeld = false;
	net->aheadHoldAfterCycle = 0;
	net->rxSize = 0;
	net->txHead = 0;
	net->txCount = 0;
	net->sessionLive = false;
	net->peerGp = 0;
	net->gpPrevSiValid = false;
	net->gpSentInitial = false;
	net->diagnosticLastSend = 0;
	net->diagnosticLastSendCycle = 0;
}

static uint32_t _newSession(struct GBASIONetlink* net) {
	uint64_t seed = (uintptr_t) net;
	if (net->d.p) {
		seed ^= mTimingGlobalTime(&net->d.p->p->timing);
	}
	seed ^= seed >> 33;
	seed *= UINT64_C(0xff51afd7ed558ccd);
	seed ^= seed >> 33;
	uint32_t session = seed ^ (seed >> 32);
	return session ? session : 1;
}

static uint16_t _mergeGp(struct GBASIONetlink* net, uint16_t local) {
	if ((local >> 14) != 2) {
		return local;
	}
	static const uint8_t peerPin[4] = { 0, 1, 3, 2 };
	uint16_t merged = local;
	for (int pin = 0; pin < 4; ++pin) {
		if (local & (1 << (pin + 4))) {
			continue;
		}
		int level = 1;
		if ((net->peerGp >> 14) == 2) {
			int source = peerPin[pin];
			if (net->peerGp & (1 << (source + 4))) {
				level = (net->peerGp >> source) & 1;
			}
		}
		merged = (merged & ~(1 << pin)) | (level << pin);
	}
	return merged;
}

static void _commitGp(struct GBASIONetlink* net, bool allowIrq) {
	if (!net->d.p || !net->gpModeActive) {
		return;
	}
	uint16_t merged = _mergeGp(net, net->localGp);
	net->d.p->rcnt = merged;
	bool si = (merged >> 2) & 1;
	if (allowIrq && (merged & 0x0100) && !(merged & 0x0040)
			&& net->gpPrevSiValid && net->gpPrevSi && !si) {
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK GP_IRQ role=%s rcnt=%04X", _roleName(net), merged);
		GBARaiseIRQ(net->d.p->p, GBA_IRQ_SIO, 0);
	}
	net->gpPrevSi = si;
	net->gpPrevSiValid = true;
}

static void _finish(struct GBASIONetlink* net) {
	struct GBASIO* sio = net->d.p;
	struct GBASIONetlinkDiagnostic* diagnostic = _diagnosticCurrent(net);
	if (diagnostic) diagnostic->finishCycle = _diagnosticCycle(net);
	sio->p->memory.io[REG_SIOMULTI0 >> 1] = net->received[0];
	sio->p->memory.io[REG_SIOMULTI1 >> 1] = net->received[1];
	sio->p->memory.io[REG_SIOMULTI2 >> 1] = 0xFFFF;
	sio->p->memory.io[REG_SIOMULTI3 >> 1] = 0xFFFF;
	sio->rcnt |= 1;
	sio->siocnt = GBASIOMultiplayerClearBusy(sio->siocnt);
	sio->siocnt = GBASIOMultiplayerSetId(sio->siocnt,
		net->role == GBA_NETLINK_HOST ? 0 : 1);
	net->transferActive = false;
	net->transferState = GBA_NETLINK_IDLE;
	net->remoteDataReady = false;
	net->localDataSent = false;
	net->sessionLive = _isCableReady(net);
	_setCableRegisters(net, _isCableReady(net));
	if (diagnostic) diagnostic->rcntFinish = sio->rcnt;
	char detail[192];
	snprintf(detail, sizeof(detail),
		"\"session\":%u,\"seq\":%u,\"data0\":%u,\"data1\":%u,\"irq\":%s",
		net->session, net->transferSequence, net->received[0], net->received[1],
		GBASIOMultiplayerIsIrq(sio->siocnt) ? "true" : "false");
	_traceText(net, "sio_complete", detail);
	NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK SIO_COMPLETE role=%s seq=%u data=%04X,%04X,FFFF,FFFF irq=%i",
		_roleName(net), net->transferSequence, net->received[0], net->received[1],
		GBASIOMultiplayerIsIrq(sio->siocnt));
	if (GBASIOMultiplayerIsIrq(sio->siocnt)) {
		snprintf(detail, sizeof(detail), "\"session\":%u,\"seq\":%u", net->session, net->transferSequence);
		_traceText(net, "sio_irq", detail);
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK SIO_IRQ role=%s seq=%u", _roleName(net), net->transferSequence);
		GBARaiseIRQ(sio->p, GBA_IRQ_SIO, 0);
	}
}

static void _startJoinTransfer(struct GBASIONetlink* net, const uint8_t* frame) {
	uint32_t sequence = _get32(&frame[12]);
	if (sequence <= net->transferSequence) {
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK STALE_START role=join seq=%u current=%u", sequence, net->transferSequence);
		return;
	}
	if (sequence != net->transferSequence + 1) {
		_close(net, "future start sequence");
		return;
	}
	if (!net->d.p || net->d.p->mode != SIO_MULTI) {
		// The slave has already left this game-level MULTI epoch. A real
		// master's clock still completes with an absent slave slot (FFFF), so
		// consume the sequence and return terminal DATA without re-entering
		// MULTI or raising a local IRQ. Silently dropping this START leaves the
		// host frozen forever at its hardware transfer boundary.
		net->transferSequence = sequence;
		_diagnosticState(net, GBA_NETLINK_STATE_START_DISCARD, _get16(&frame[16]));
		_queue(net, NETLINK_DATA, sequence, 0xFFFF, net->d.p ? net->d.p->siocnt : 0);
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK TERMINAL_DATA role=join seq=%u value=FFFF mode=%u", sequence,
			net->d.p ? net->d.p->mode : 0);
		return;
	}
	if (net->transferActive) {
		if (net->hasPendingStart) {
			_close(net, "multiple pending starts");
			return;
		}
		memcpy(net->pendingStart, frame, GBA_NETLINK_FRAME_SIZE);
		net->hasPendingStart = true;
		_diagnosticState(net, GBA_NETLINK_STATE_START_QUEUED, _get16(&frame[16]));
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK QUEUE_NEXT_START role=join seq=%u", sequence);
		return;
	}
	struct GBASIONetlinkDiagnostic* diagnostic = &net->diagnostic[net->diagnosticCurrent];
	if (!diagnostic->valid || diagnostic->session != net->session || diagnostic->sequence != sequence) {
		diagnostic = _diagnosticStart(net, sequence, _get16(&frame[16]), net->d.p->siocnt);
	}
	net->transferSequence = sequence;
	net->transferActive = true;
	net->transferState = GBA_NETLINK_JOIN_WAIT_CLOCK;
	net->localDataSent = false;
	net->transferStartClock = _get64(&frame[20]);
	net->transferFinishClock = net->transferStartClock + _transferCyclesFor(_get16(&frame[18]));
	if (net->transferFinishClock > net->peerHorizon) net->peerHorizon = net->transferFinishClock;
	net->peerStartTime = 0;
	net->transferCycles = _transferCyclesFor(_get16(&frame[18]));
	net->received[0] = _get16(&frame[16]);
	net->received[1] = 0xFFFF;
	diagnostic->rcntStart = net->d.p->rcnt;
	NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK START role=join seq=%u host=%04X hostGap=%u localGap=%u cycles=%u siocnt=%04X",
		sequence, net->received[0], net->peerStartTime, net->linkTime,
		net->transferCycles, _get16(&frame[18]));
}

static void _handlePeerMode(struct GBASIONetlink* net, const uint8_t* frame) {
	uint16_t value = _get16(&frame[16]);
	if (value > 1) {
		_close(net, "invalid multi mode state");
		return;
	}
	bool active = value != 0;
	bool changed = net->peerMultiActive != active;
	net->peerMultiActive = active;
	_diagnosticState(net, GBA_NETLINK_STATE_PEER_MODE, value);
	NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK MODE role=%s peerActive=%i seq=%u",
		_roleName(net), active, _get32(&frame[12]));

	if (!active) {
		// Keep an already clocked Host transfer alive long enough to receive
		// the terminal FFFF DATA sent for the crossed START. With no transfer
		// in flight, fully return the game-side cable state to its pre-room
		// epoch while preserving the TCP session.
		if (!net->transferActive) {
			_resetCableEpoch(net);
		} else {
			net->sessionLive = false;
			net->aheadHoldAfterCycle = 0;
		}
		_setCableRegisters(net, false);
		return;
	}

	if (changed && net->localMultiActive) {
		_resetCableEpoch(net);
		net->lastCycle = _diagnosticCycle(net);
		// Reply once on a false->true transition. This makes re-entry a fresh
		// two-sided epoch even if one game reached MULTI before the other.
		_publishMultiMode(net);
	}
	_setCableRegisters(net, _isCableReady(net));
}

static void _handle(struct GBASIONetlink* net) {
	uint8_t* frame = net->rx;
	uint8_t type = frame[5];
	uint8_t peerRole = frame[6];
	uint32_t session = _get32(&frame[8]);
	uint32_t sequence = _get32(&frame[12]);
	if (memcmp(frame, "MGNL", 4) || frame[4] != NETLINK_PROTOCOL) {
		_close(net, "protocol mismatch");
		return;
	}
	if (frame[7] != 2 || peerRole == net->role || peerRole > GBA_NETLINK_JOIN) {
		_close(net, "peer role mismatch");
		return;
	}
	if (type == NETLINK_HELLO) {
		if (net->role != GBA_NETLINK_HOST || session || _connectionState(net) != GBA_NETLINK_HANDSHAKE) {
			_close(net, "unexpected hello");
			return;
		}
		_advanceLinkTime(net, &net->d.p->p->timing);
		net->syncClock = 0;
		net->session = _newSession(net);
		_queue(net, NETLINK_WELCOME, 0, 0, 0);
		NETLINK_DEBUG_LOG(GBA_SIO, INFO, "NETLINK HANDSHAKE role=host session=%08X", net->session);
		return;
	}
	if (type == NETLINK_WELCOME) {
		if (net->role != GBA_NETLINK_JOIN || !session || _connectionState(net) != GBA_NETLINK_HANDSHAKE) {
			_close(net, "unexpected welcome");
			return;
		}
		net->session = session;
		net->syncClock = net->peerHorizon = _get64(&frame[20]);
		net->lastCycle = _diagnosticCycle(net);
		net->linkTime = 0;
		_queue(net, NETLINK_READY_FRAME, 0, 0, 0);
		_setConnectionState(net, GBA_NETLINK_READY);
		_setCableRegisters(net, _isCableReady(net));
		_publishMultiMode(net);
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK CLOCK_SYNC role=join clock=%u", net->linkTime);
		NETLINK_DEBUG_LOG(GBA_SIO, INFO, "NETLINK CONNECTED role=join session=%08X", net->session);
		return;
	}
	if (type == NETLINK_READY_FRAME) {
		if (net->role != GBA_NETLINK_HOST || !net->session || session != net->session
				|| _connectionState(net) != GBA_NETLINK_HANDSHAKE) {
			_close(net, "unexpected ready");
			return;
		}
		_setConnectionState(net, GBA_NETLINK_READY);
		_setCableRegisters(net, _isCableReady(net));
		_publishMultiMode(net);
		NETLINK_DEBUG_LOG(GBA_SIO, INFO, "NETLINK CONNECTED role=host session=%08X", net->session);
		return;
	}
	if (!_isReady(net) || !net->session || session != net->session) {
		_close(net, "session mismatch");
		return;
	}
	switch (type) {
 case NETLINK_CLOCK:
  if (net->role != GBA_NETLINK_JOIN) { _close(net, "clock sent to host"); return; }
  if (_get64(&frame[20]) > net->peerHorizon) net->peerHorizon = _get64(&frame[20]);
  break;
	case NETLINK_START:
		if (net->role != GBA_NETLINK_JOIN) {
			_close(net, "start sent to host");
			return;
		}
		_startJoinTransfer(net, frame);
		break;
	case NETLINK_DATA:
		if (net->role == GBA_NETLINK_HOST && !net->transferActive
				&& sequence <= net->transferSequence) {
			NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK DISCARD_DATA role=host seq=%u current=%u mode=%u",
				sequence, net->transferSequence, net->d.p ? net->d.p->mode : 0);
			break;
		}
		if (net->role != GBA_NETLINK_HOST || !net->transferActive
				|| sequence != net->transferSequence || net->remoteDataReady) {
			_close(net, "unexpected data sequence");
			return;
		}
		if (net->performanceFile && net->performanceRoundStartUs) {
			uint64_t nowUs = net->performanceClockUs();
			uint64_t elapsed = nowUs - net->performanceRoundStartUs;
			if (net->performanceStartSentUs) {
				uint64_t replyUs = nowUs - net->performanceStartSentUs;
				++net->performanceSocketDataCount;
				net->performanceSocketDataTotalUs += replyUs;
				if (replyUs > net->performanceSocketDataMaxUs) net->performanceSocketDataMaxUs = replyUs;
				net->performanceStartSentUs = 0;
			}
			++net->performanceRoundCount;
			net->performanceRoundTotalUs += elapsed;
			if (elapsed > net->performanceRoundMaxUs) net->performanceRoundMaxUs = elapsed;
			net->performanceRoundStartUs = 0;
		}
		net->received[1] = _get16(&frame[16]);
		{
			struct GBASIONetlinkDiagnostic* diagnostic = _diagnosticCurrent(net);
			if (diagnostic) diagnostic->remoteValue = net->received[1];
		}
		net->remoteDataReady = true;
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK DATA role=host seq=%u value=%04X localGap=%u",
			sequence, net->received[1], net->linkTime);
		break;
	case NETLINK_GP:
		net->peerGp = _get16(&frame[16]);
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK GP role=%s peerRcnt=%04X", _roleName(net), net->peerGp);
		_commitGp(net, true);
		break;
	case NETLINK_MODE:
		_handlePeerMode(net, frame);
		break;
	case NETLINK_CLOSE:
		_close(net, "peer requested close");
		break;
	default:
		_close(net, "unexpected frame type");
		break;
	}
}

static void _receive(struct GBASIONetlink* net) {
 while (!SOCKET_FAILED(net->socket) && !net->closing) {
  if (net->rxSize == GBA_NETLINK_FRAME_SIZE) {
   uint8_t type = net->rx[5];
   // Keep master events in TCP order until their emulated timestamp.
   // In particular, do not apply a future MODE before an older transfer.
   if (_isReady(net) && net->role == GBA_NETLINK_JOIN
     && net->rx[4] == NETLINK_PROTOCOL
     && _get32(&net->rx[8]) == net->session
     && (type == NETLINK_START || type == NETLINK_MODE || type == NETLINK_GP)) {
    uint64_t when = _get64(&net->rx[20]);
    if (when > net->peerHorizon) net->peerHorizon = when;
    if (net->syncClock < when) return;
   }
   _traceFrame(net, "rx", net->rx);
   _handle(net);
   net->rxSize = 0;
   continue;
  }
  ssize_t got = SocketRecv(net->socket, net->rx + net->rxSize,
    GBA_NETLINK_FRAME_SIZE - net->rxSize);
  if (got > 0) {
   size_t remaining = GBA_NETLINK_FRAME_SIZE - net->rxSize;
   if ((size_t) got < remaining) _traceChunk(net, "rx", got, net->rxSize);
   net->rxSize += got;
   continue;
  }
  if (!got) _close(net, "peer disconnected");
  else if (!SocketWouldBlock()) {
   net->performanceSocketError = SocketError();
   _close(net, "receive failed");
  }
  break;
 }
}

static void _tcpConnected(struct GBASIONetlink* net) {
	net->connectingSocket = false;
	SocketSetBlocking(net->socket, false);
	SocketSetTCPPush(net->socket, true);
	_setConnectionState(net, GBA_NETLINK_HANDSHAKE);
	if (net->role == GBA_NETLINK_JOIN) {
		_queue(net, NETLINK_HELLO, 0, 0, 0);
	}
	NETLINK_DEBUG_LOG(GBA_SIO, INFO, "NETLINK TCP_CONNECTED role=%s", _roleName(net));
}

static void _beginConnect(struct GBASIONetlink* net, uint64_t now) {
	bool connected = false;
	net->socket = SocketConnectTCPNonBlocking(net->remotePort, &net->remoteAddress, &connected);
	if (SOCKET_FAILED(net->socket)) {
		net->performanceSocketError = SocketError();
		_performanceReport(net, "connect_failed", false);
		net->nextConnectAttempt = now + NETLINK_CONNECT_RETRY_CYCLES;
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK CONNECT_RETRY role=join");
		return;
	}
	net->connectingSocket = !connected;
	if (connected) {
		_tcpConnected(net);
	}
}

static void _pollConnect(struct GBASIONetlink* net, uint64_t now) {
	if (SOCKET_FAILED(net->socket)) {
		if (now >= net->nextConnectAttempt) {
			_beginConnect(net, now);
		}
		return;
	}
	if (!net->connectingSocket) {
		return;
	}
	Socket writable[1] = { net->socket };
	Socket errors[1] = { net->socket };
	if (SocketPoll(1, NULL, writable, errors, 0) <= 0) {
		return;
	}
	int error = SocketGetConnectionError(net->socket);
	if (!error) {
		_tcpConnected(net);
		return;
	}
	net->performanceSocketError = error;
	_performanceReport(net, "connect_failed", false);
	SocketClose(net->socket);
	net->socket = INVALID_SOCKET;
	net->connectingSocket = false;
	net->nextConnectAttempt = now + NETLINK_CONNECT_RETRY_CYCLES;
	NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK CONNECT_RETRY role=join error=%i", error);
}

static void _accept(struct GBASIONetlink* net) {
	if (net->role != GBA_NETLINK_HOST || SOCKET_FAILED(net->listener)
			|| !SOCKET_FAILED(net->socket) || _connectionState(net) != GBA_NETLINK_LISTENING) {
		return;
	}
	Socket accepted = SocketAccept(net->listener, NULL);
	if (SOCKET_FAILED(accepted)) {
		return;
	}
	_resetPeerSession(net);
	net->socket = accepted;
	_tcpConnected(net);
}

static bool _waitForSocket(struct GBASIONetlink* net, int timeoutMs) {
	if (SOCKET_FAILED(net->socket)) {
		return false;
	}
	Socket readable[1] = { net->socket };
	int result = SocketPoll(1, readable, NULL, NULL, timeoutMs);
	if (result > 0) {
		_receive(net);
		return true;
	}
	return false;
}

void GBASIONetlinkCreate(struct GBASIONetlink* net) {
	memset(net, 0, sizeof(*net));
	net->listener = INVALID_SOCKET;
	net->socket = INVALID_SOCKET;
	net->d.init = _init;
	net->d.deinit = _deinit;
	net->d.load = _load;
	net->d.unload = _unload;
	net->d.writeRegister = _write;
	net->rcnt.parent = net;
	net->rcnt.d.init = _rcntInit;
	net->rcnt.d.deinit = _rcntDeinit;
	net->rcnt.d.writeRegister = _rcntWrite;
	net->event.context = net;
	net->event.name = "GBA SIO Netlink";
	net->event.callback = _process;
	net->event.priority = 0x80;
	_setConnectionState(net, GBA_NETLINK_DISCONNECTED);
}

void GBASIONetlinkDestroy(struct GBASIONetlink* net) {
	_performanceReport(net, "local_disconnect", true);
	_diagnosticDump(net);
	_sendCloseBestEffort(net);
	if (net->d.p && mTimingIsScheduled(&net->d.p->p->timing, &net->event)) {
		mTimingDeschedule(&net->d.p->p->timing, &net->event);
	}
	if (!SOCKET_FAILED(net->socket)) {
		SocketClose(net->socket);
	}
	if (!SOCKET_FAILED(net->listener)) {
		SocketClose(net->listener);
	}
	net->socket = INVALID_SOCKET;
	net->listener = INVALID_SOCKET;
	net->transportActive = false;
	_setConnectionState(net, GBA_NETLINK_DISCONNECTED);
	_traceText(net, "trace_end", NULL);
	if (net->performanceFile) {
		fclose(net->performanceFile);
		net->performanceFile = NULL;
	}
	if (net->traceFile) {
		fclose(net->traceFile);
		net->traceFile = NULL;
	}
}

bool GBASIONetlinkHost(struct GBASIONetlink* net, int port) {
	net->role = GBA_NETLINK_HOST;
	net->listener = SocketOpenTCP(port, NULL);
	if (SOCKET_FAILED(net->listener) || SocketListen(net->listener, 1) < 0) {
		_setError(net, "could not listen on TCP port");
		GBASIONetlinkDestroy(net);
		_setConnectionState(net, GBA_NETLINK_ERROR);
		return false;
	}
	SocketSetBlocking(net->listener, false);
	_setConnectionState(net, GBA_NETLINK_LISTENING);
	return true;
}

bool GBASIONetlinkJoin(struct GBASIONetlink* net, const char* host, int port) {
	net->role = GBA_NETLINK_JOIN;
	if (SocketResolveHost(host, &net->remoteAddress)) {
		_setError(net, "could not resolve host");
		_setConnectionState(net, GBA_NETLINK_ERROR);
		return false;
	}
	net->remotePort = port;
	net->nextConnectAttempt = 0;
	_setConnectionState(net, GBA_NETLINK_CONNECTING);
	return true;
}

bool GBASIONetlinkSetTracePath(struct GBASIONetlink* net, const char* path) {
	if (net->traceFile) {
		fclose(net->traceFile);
		net->traceFile = NULL;
	}
	net->traceOrder = 0;
	if (!path || !path[0]) return true;
	net->traceFile = fopen(path, "w");
	if (!net->traceFile) return false;
	setvbuf(net->traceFile, NULL, _IOFBF, 1024 * 1024);
	_traceText(net, "trace_start", "\"protocol\":6,\"frame_size\":28");
	return true;
}

bool GBASIONetlinkSetDiagnosticPath(struct GBASIONetlink* net, const char* path) {
	net->diagnosticPath[0] = '\0';
	if (!path || !path[0]) return true;
	if (strlen(path) >= sizeof(net->diagnosticPath)) return false;
	strcpy(net->diagnosticPath, path);
	return true;
}

enum GBASIONetlinkConnectionState GBASIONetlinkGetState(const struct GBASIONetlink* net) {
	return _connectionState(net);
}

const char* GBASIONetlinkGetError(const struct GBASIONetlink* net) {
	return net->lastError;
}

static bool _init(struct GBASIODriver* driver) {
	struct GBASIONetlink* net = (struct GBASIONetlink*) driver;
	net->transportActive = true;
	net->lastCycle = mTimingGlobalTime(&driver->p->p->timing);
	_setCableRegisters(net, _isCableReady(net));
	if (!mTimingIsScheduled(&driver->p->p->timing, &net->event)) {
		mTimingSchedule(&driver->p->p->timing, &net->event, 0);
	}
	return true;
}

static void _deinit(struct GBASIODriver* driver) {
	struct GBASIONetlink* net = (struct GBASIONetlink*) driver;
	net->transportActive = false;
	if (mTimingIsScheduled(&driver->p->p->timing, &net->event)) {
		mTimingDeschedule(&driver->p->p->timing, &net->event);
	}
}

static bool _load(struct GBASIODriver* driver) {
	struct GBASIONetlink* net = (struct GBASIONetlink*) driver;
	_advanceLinkTime(net, &driver->p->p->timing);
	// Returning to MULTI begins a new cable-clock epoch. Time spent in GP,
	// Normal or UART mode must not count as silence in the old transfer
	// stream or immediately trigger the join-side ahead hold.
	net->localMultiActive = true;
	_resetCableEpoch(net);
	net->lastCycle = mTimingGlobalTime(&driver->p->p->timing);
	_setCableRegisters(net, _isCableReady(net));
	_diagnosticState(net, GBA_NETLINK_STATE_LOAD, 0);
	_publishMultiMode(net);
	return true;
}

static bool _unload(struct GBASIODriver* driver) {
	struct GBASIONetlink* net = (struct GBASIONetlink*) driver;
	_advanceLinkTime(net, &driver->p->p->timing);
	// Leaving MULTI ends the current transfer stream. The TCP session stays
	// connected for a later return, but no SIO pacing wait may survive into
	// another hardware mode (Pokémon leaves MULTI while exiting the room).
	net->localMultiActive = false;
	// Re-entering MULTI must require a fresh active announcement from the
	// peer, not the remembered state from the room that just ended.
	net->peerMultiActive = false;
	_resetCableEpoch(net);
	net->lastCycle = mTimingGlobalTime(&driver->p->p->timing);
	_diagnosticState(net, GBA_NETLINK_STATE_UNLOAD, 0);
	_publishMultiMode(net);
	return true;
}

static bool _rcntInit(struct GBASIODriver* driver) {
	UNUSED(driver);
	return true;
}

static void _rcntDeinit(struct GBASIODriver* driver) {
	UNUSED(driver);
}

static uint16_t _rcntWrite(struct GBASIODriver* driver, uint32_t address, uint16_t value) {
	struct GBASIONetlinkRCNT* observer = (struct GBASIONetlinkRCNT*) driver;
	struct GBASIONetlink* net = observer->parent;
	if (net->d.p) _advanceLinkTime(net, &net->d.p->p->timing);
	if (address != REG_RCNT) {
		return value;
	}
	bool wasGp = net->gpModeActive;
	bool nowGp = (value >> 14) == 2;
	uint16_t committed = net->d.p ? net->d.p->rcnt : value;
	if (nowGp) {
		// In GP mode the low nibble is the CPU's output latch/input request;
		// the generic mGBA RCNT path deliberately leaves it to the driver.
		committed = value & 0xC1FF;
		if (net->d.p) {
			net->d.p->rcnt = committed;
		}
	}
	uint16_t previous = net->localGp;
	if (previous != committed || wasGp != nowGp) {
		char detail[128];
		snprintf(detail, sizeof(detail), "\"value\":%u,\"previous\":%u,\"mode\":%u", committed, previous, committed >> 14);
		_traceText(net, "rcnt_write", detail);
	}
	net->localGp = committed;
	net->gpModeActive = nowGp;
	if (nowGp && !wasGp) {
		net->gpPrevSiValid = false;
		_cancelTransfer(net, false);
	}
	if (nowGp != wasGp) {
		_diagnosticState(net, nowGp ? GBA_NETLINK_STATE_GP_ENTER : GBA_NETLINK_STATE_GP_LEAVE,
			committed);
	}
	if (_isReady(net) && (!net->gpSentInitial || previous != committed)) {
		_queue(net, NETLINK_GP, 0, committed, 0);
		net->gpSentInitial = true;
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK GP_SEND role=%s rcnt=%04X", _roleName(net), committed);
	}
	if (nowGp) {
		_commitGp(net, false);
	}
	return value;
}

static uint16_t _write(struct GBASIODriver* driver, uint32_t address, uint16_t value) {
	struct GBASIONetlink* net = (struct GBASIONetlink*) driver;
	_advanceLinkTime(net, &net->d.p->p->timing);
	if (address == REG_SIOMLT_SEND) {
		net->diagnosticLastSend = value;
		net->diagnosticLastSendCycle = _diagnosticCycle(net);
		return value;
	}
	if (address != REG_SIOCNT) {
		return value;
	}
	char detail[128];
	snprintf(detail, sizeof(detail), "\"value\":%u,\"mode\":%u,\"start\":%s", value,
		net->d.p ? net->d.p->mode : 0, (value & 0x0080) ? "true" : "false");
	_traceText(net, "siocnt_write", detail);
	_setCableRegisters(net, _isCableReady(net));
	if (!net->stateDiagnosticLastSiocntWriteValid || net->stateDiagnosticLastSiocntWrite != value) {
		net->stateDiagnosticLastSiocntWrite = value;
		net->stateDiagnosticLastSiocntWriteValid = true;
		_diagnosticState(net, GBA_NETLINK_STATE_SIOCNT_WRITE, value);
	}
	if ((value & 0x0080) && net->role == GBA_NETLINK_HOST
			&& _isCableReady(net) && !net->transferActive) {
		_advanceLinkTime(net, &net->d.p->p->timing);
		_normaliseClock(net);
		net->transferActive = true;
		net->transferState = GBA_NETLINK_HOST_WAIT_PEER;
		net->remoteDataReady = false;
		net->localDataSent = true;
		net->transferSequence = ++net->sequence;
		if (net->performanceFile) {
			net->performanceRoundStartUs = net->performanceClockUs();
			net->performanceStartSentUs = 0;
		}
		net->received[0] = net->d.p->p->memory.io[REG_SIOMLT_SEND >> 1];
		net->received[1] = 0xFFFF;
		// Match cable hardware visibility during an active transfer:
		// publish the master's slot immediately and leave absent/unreceived
		// slots high until the transfer commits.
		net->d.p->p->memory.io[REG_SIOMULTI0 >> 1] = net->received[0];
		net->d.p->p->memory.io[REG_SIOMULTI1 >> 1] = 0xFFFF;
		net->d.p->p->memory.io[REG_SIOMULTI2 >> 1] = 0xFFFF;
		net->d.p->p->memory.io[REG_SIOMULTI3 >> 1] = 0xFFFF;
		net->peerStartTime = net->linkTime;
		net->transferCycles = _transferCyclesFor(value);
		net->linkTime = 0;
		// A transfer may start between transport timing callbacks. Reset the
		// accumulator origin together with linkTime so the next callback does
		// not count pre-START idle cycles toward the hardware transfer time.
		net->lastCycle = mTimingGlobalTime(&net->d.p->p->timing);
		net->d.p->siocnt = GBASIOMultiplayerFillBusy(net->d.p->siocnt);
		_setCableRegisters(net, true);
		struct GBASIONetlinkDiagnostic* diagnostic = _diagnosticStart(net, net->transferSequence,
			net->received[0], value);
		diagnostic->latchedValue = net->received[0];
		diagnostic->latchCycle = diagnostic->startCycle;
		diagnostic->modeLatch = net->d.p->mode;
		diagnostic->siocntLatch = net->d.p->siocnt;
		diagnostic->rcntLatch = net->d.p->rcnt;
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK START role=host seq=%u value=%04X hostGap=%u cycles=%u siocnt=%04X",
			net->transferSequence, net->received[0], net->peerStartTime,
			net->transferCycles, value);
		_queue(net, NETLINK_START, net->transferSequence, net->received[0], value);
	}
	value &= 0xFF83;
	value |= net->d.p->siocnt & 0x00FC;
	return value;
}

static void _serviceTransfer(struct GBASIONetlink* net) {
	if (net->role == GBA_NETLINK_JOIN && !net->transferActive && net->hasPendingStart
			&& net->d.p->mode == SIO_MULTI) {
		uint8_t frame[GBA_NETLINK_FRAME_SIZE];
		memcpy(frame, net->pendingStart, sizeof(frame));
		net->hasPendingStart = false;
		_startJoinTransfer(net, frame);
	}
	if (net->role == GBA_NETLINK_JOIN && net->transferActive && !net->localDataSent) {
		if (net->syncClock >= net->transferStartClock) {
   // A late start cannot be reconstructed from today's send register.
   // Allow only scheduler instruction overshoot, never whole-frame catchup.
   if (net->syncClock - net->transferStartClock > 128) {
    _close(net, "late serial start"); return;
   }
			// The master's word is visible in SIOMULTI0 as soon as the serial clock
			// reaches the slave. Do not defer every receive register until completion:
			// games may poll SIOMULTI while Busy is still set.
			net->d.p->p->memory.io[REG_SIOMULTI0 >> 1] = net->received[0];
			net->d.p->p->memory.io[REG_SIOMULTI1 >> 1] = 0xFFFF;
			net->d.p->p->memory.io[REG_SIOMULTI2 >> 1] = 0xFFFF;
			net->d.p->p->memory.io[REG_SIOMULTI3 >> 1] = 0xFFFF;
			net->d.p->siocnt = GBASIOMultiplayerFillBusy(net->d.p->siocnt);
			_setCableRegisters(net, true);

			net->received[1] = net->d.p->p->memory.io[REG_SIOMLT_SEND >> 1];
			struct GBASIONetlinkDiagnostic* diagnostic = _diagnosticCurrent(net);
			if (diagnostic) {
				diagnostic->latchedValue = net->received[1];
				diagnostic->latchCycle = _diagnosticCycle(net);
				diagnostic->startCycle = diagnostic->latchCycle;
				diagnostic->modeLatch = net->d.p->mode;
				diagnostic->siocntLatch = net->d.p->siocnt;
				diagnostic->rcntLatch = net->d.p->rcnt;
			}
			net->localDataSent = true;
			net->transferState = GBA_NETLINK_TRANSFER;
			NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK DATA role=join seq=%u value=%04X",
				net->transferSequence, net->received[1]);
			_queue(net, NETLINK_DATA, net->transferSequence, net->received[1], net->d.p->siocnt);
		}
	}
	if (net->role == GBA_NETLINK_JOIN && net->transferActive && net->localDataSent
			&& net->syncClock >= net->transferFinishClock) {
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK FINISH role=join seq=%u cycles=%u",
			net->transferSequence, net->linkTime);
		net->linkTime = 0;
		_finish(net);
	}
	if (net->role == GBA_NETLINK_HOST && net->transferActive && net->remoteDataReady
			&& net->linkTime >= net->transferCycles) {
		NETLINK_DEBUG_LOG(GBA_SIO, DEBUG, "NETLINK FINISH role=host seq=%u cycles=%u",
			net->transferSequence, net->linkTime);
		net->linkTime -= net->transferCycles;
		_finish(net);
	}
}

static void _serviceClosing(struct GBASIONetlink* net, struct mTiming* timing) {
	if (!SOCKET_FAILED(net->socket)) {
		SocketClose(net->socket);
		net->socket = INVALID_SOCKET;
	}
	_cancelTransfer(net, true);
	net->rxSize = 0;
	net->txHead = 0;
	net->txCount = 0;
	net->session = 0;
	net->sessionLive = false;
	net->peerGp = 0;
	net->gpSentInitial = false;
	_setCableRegisters(net, false);
	if (net->role == GBA_NETLINK_HOST && !SOCKET_FAILED(net->listener)) {
		net->closing = false;
		_setConnectionState(net, GBA_NETLINK_LISTENING);
		NETLINK_DEBUG_LOG(GBA_SIO, INFO, "NETLINK LISTENING role=host after peer disconnect");
		if (net->transportActive) {
			mTimingSchedule(timing, &net->event, NETLINK_IDLE_POLL_CYCLES);
		}
	} else {
		_setConnectionState(net, GBA_NETLINK_ERROR);
	}
}

static bool _shouldHoldAhead(const struct GBASIONetlink* net) {
 return net->role == GBA_NETLINK_JOIN && _isReady(net)
  && net->syncClock >= net->peerHorizon;
}

static void _process(struct mTiming* timing, void* user, uint32_t cyclesLate) {
	UNUSED(cyclesLate);
	struct GBASIONetlink* net = user;
	_advanceLinkTime(net, timing);
	if (net->closing) {
		_serviceClosing(net, timing);
		return;
	}

	enum GBASIONetlinkConnectionState connection = _connectionState(net);
	if (connection == GBA_NETLINK_LISTENING) {
		_accept(net);
	} else if (connection == GBA_NETLINK_CONNECTING) {
		_pollConnect(net, mTimingGlobalTime(timing));
	}
	if (!SOCKET_FAILED(net->socket) && !net->connectingSocket) {
		_flush(net);
		_receive(net);
		_flush(net);
	}
	if (net->closing) {
		_serviceClosing(net, timing);
		return;
	}

	if (_isReady(net)) {
		if (!net->gpSentInitial) {
			net->localGp = net->d.p ? net->d.p->rcnt : 0;
			_queue(net, NETLINK_GP, 0, net->localGp, 0);
			net->gpSentInitial = true;
		}
		_commitGp(net, true);
		_serviceTransfer(net);
		_flush(net);
	}
	if (net->closing) {
		_serviceClosing(net, timing);
		return;
	}

	// Publish progress even with no serial transfers, so room teardown and
 // ordinary game execution do not depend on an IRQ-enabled grace window.
 if (net->role == GBA_NETLINK_HOST && _isReady(net)
   && net->syncClock - net->publishedClock >= 1024) {
  _queue(net, NETLINK_CLOCK, net->transferSequence, 0, 0);
  net->publishedClock = net->syncClock;
 }
	bool waitAtBoundary = net->role == GBA_NETLINK_HOST && net->transferActive
		&& net->linkTime >= net->transferCycles && !net->remoteDataReady;
	bool holdAhead = _shouldHoldAhead(net);
	net->clockHeld = waitAtBoundary || holdAhead;
	_diagnosticHold(net, holdAhead);
	if ((waitAtBoundary || holdAhead) && _isReady(net)) {
		uint64_t startUs = net->performanceFile ? net->performanceClockUs() : 0;
		_waitForSocket(net, 1);
		if (net->performanceFile) {
			uint64_t elapsed = net->performanceClockUs() - startUs;
			if (waitAtBoundary) net->performanceHostWaitUs += elapsed;
			else net->performanceJoinWaitUs += elapsed;
			++net->performanceWaitCalls;
			_performanceReport(net, "summary", false);
		}
		if (!net->closing && _isReady(net)) _serviceTransfer(net);
		_flush(net);
		if (net->transportActive && !net->closing) {
			mTimingSchedule(timing, &net->event, 1);
		}
		return;
	}

	if (!net->transportActive) {
		return;
	}
	int32_t next = NETLINK_IDLE_POLL_CYCLES;
	connection = _connectionState(net);
	if (net->transferActive || net->hasPendingStart) {
		next = NETLINK_ACTIVE_POLL_CYCLES;
	} else if (connection == GBA_NETLINK_CONNECTING || connection == GBA_NETLINK_HANDSHAKE) {
		next = NETLINK_HANDSHAKE_POLL_CYCLES;
	}
 if (net->role == GBA_NETLINK_JOIN && _isReady(net)) {
  uint64_t target = net->peerHorizon;
  if (net->transferActive) target = net->localDataSent ? net->transferFinishClock : net->transferStartClock;
  if (target > net->syncClock && target - net->syncClock < (uint64_t) next)
   next = (int32_t) (target - net->syncClock);
 }
	mTimingSchedule(timing, &net->event, next);
}
