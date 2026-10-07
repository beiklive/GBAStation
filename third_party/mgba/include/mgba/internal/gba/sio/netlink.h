/* SPDX-License-Identifier: MPL-2.0 */
#ifndef GBA_SIO_NETLINK_H
#define GBA_SIO_NETLINK_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/core/timing.h>
#include <mgba/internal/gba/sio.h>
#include <mgba-util/socket.h>
#include <stdio.h>

#define GBA_NETLINK_FRAME_SIZE 28
#define GBA_NETLINK_TX_CAPACITY 16
#define GBA_NETLINK_DIAGNOSTIC_CAPACITY 512
#define GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY 256

enum GBASIONetlinkRole {
	GBA_NETLINK_HOST = 0,
	GBA_NETLINK_JOIN = 1,
};

enum GBASIONetlinkConnectionState {
	GBA_NETLINK_DISCONNECTED = 0,
	GBA_NETLINK_LISTENING,
	GBA_NETLINK_CONNECTING,
	GBA_NETLINK_HANDSHAKE,
	GBA_NETLINK_READY,
	GBA_NETLINK_ERROR,
};

enum GBASIONetlinkTransferState {
	GBA_NETLINK_IDLE = 0,
	GBA_NETLINK_HOST_WAIT_PEER,
	GBA_NETLINK_JOIN_WAIT_CLOCK,
	GBA_NETLINK_TRANSFER,
};

struct GBASIONetlinkTxFrame {
	uint8_t data[GBA_NETLINK_FRAME_SIZE];
	size_t offset;
};

struct GBASIONetlinkDiagnostic {
	uint32_t session;
	uint32_t sequence;
	uint64_t sendWriteCycle;
	uint64_t startCycle;
	uint64_t latchCycle;
	uint64_t finishCycle;
	uint16_t hostValue;
	uint16_t lastSendWrite;
	uint16_t latchedValue;
	uint16_t remoteValue;
	uint16_t siocntStart;
	uint16_t siocntLatch;
	uint16_t rcntStart;
	uint16_t rcntLatch;
	uint16_t rcntFinish;
	uint8_t modeStart;
	uint8_t modeLatch;
	bool valid;
};

enum GBASIONetlinkStateDiagnosticEvent {
	GBA_NETLINK_STATE_LOAD = 1,
	GBA_NETLINK_STATE_UNLOAD,
	GBA_NETLINK_STATE_GP_ENTER,
	GBA_NETLINK_STATE_GP_LEAVE,
	GBA_NETLINK_STATE_HOLD_ENTER,
	GBA_NETLINK_STATE_HOLD_EXIT,
	GBA_NETLINK_STATE_SIOCNT_WRITE,
	GBA_NETLINK_STATE_START_DISCARD,
	GBA_NETLINK_STATE_START_QUEUED,
	GBA_NETLINK_STATE_CLOSE,
	GBA_NETLINK_STATE_PEER_MODE,
};

struct GBASIONetlinkStateDiagnostic {
	uint64_t cycle;
	int32_t linkTime;
	uint32_t sequence;
	uint16_t value;
	uint16_t rcnt;
	uint16_t siocnt;
	uint8_t event;
	uint8_t mode;
	uint8_t connection;
	bool transferActive;
	bool sessionLive;
	bool pendingStart;
	bool localMultiActive;
	bool peerMultiActive;
};

struct GBASIONetlink;

struct GBASIONetlinkRCNT {
	struct GBASIODriver d;
	struct GBASIONetlink* parent;
};

struct GBASIONetlink {
	struct GBASIODriver d;
	struct GBASIONetlinkRCNT rcnt;
	struct mTimingEvent event;
	Socket listener;
	Socket socket;
	struct Address remoteAddress;
	int remotePort;
	enum GBASIONetlinkRole role;
	enum GBASIONetlinkTransferState transferState;
	volatile int connectionState;
	uint32_t session;
	uint32_t sequence;
	uint32_t transferSequence;
	uint16_t received[4];
	uint8_t rx[GBA_NETLINK_FRAME_SIZE];
	size_t rxSize;
	uint8_t pendingStart[GBA_NETLINK_FRAME_SIZE];
	struct GBASIONetlinkTxFrame tx[GBA_NETLINK_TX_CAPACITY];
	size_t txHead;
	size_t txCount;
	uint64_t syncClock;
	uint64_t peerHorizon;
	uint64_t publishedClock;
	uint64_t transferStartClock;
	uint64_t transferFinishClock;
	bool clockHeld;
	uint64_t lastCycle;
	uint64_t aheadHoldAfterCycle;
	uint64_t nextConnectAttempt;
	int32_t linkTime;
	int32_t peerStartTime;
	int32_t transferCycles;
	uint16_t localGp;
	uint16_t peerGp;
	bool transportActive;
	bool connectingSocket;
	bool closing;
	bool transferActive;
	bool remoteDataReady;
	bool localDataSent;
	bool sessionLive;
	bool localMultiActive;
	bool peerMultiActive;
	bool gpPrevSi;
	bool gpPrevSiValid;
	bool gpModeActive;
	bool gpSentInitial;
	bool hasPendingStart;
	FILE* traceFile;
	uint64_t traceOrder;
	struct GBASIONetlinkDiagnostic diagnostic[GBA_NETLINK_DIAGNOSTIC_CAPACITY];
	size_t diagnosticNext;
	size_t diagnosticCount;
	size_t diagnosticCurrent;
	uint16_t diagnosticLastSend;
	uint64_t diagnosticLastSendCycle;
	struct GBASIONetlinkStateDiagnostic stateDiagnostic[GBA_NETLINK_STATE_DIAGNOSTIC_CAPACITY];
	size_t stateDiagnosticNext;
	size_t stateDiagnosticCount;
	bool stateDiagnosticHoldActive;
	uint16_t stateDiagnosticLastSiocntWrite;
	bool stateDiagnosticLastSiocntWriteValid;
	char diagnosticPath[1024];
	FILE* performanceFile;
	uint64_t (*performanceClockUs)(void);
	uint64_t performanceReportUs;
	uint64_t performanceHostWaitUs;
	uint64_t performanceJoinWaitUs;
	uint64_t performanceReportedWaitUs;
	uint64_t performanceWaitCalls;
	uint64_t performanceFrames;
	uint64_t performanceReportedFrames;
	uint64_t performanceLastFrameUs;
	uint64_t performanceMaxFrameGapUs;
	uint64_t performanceRoundStartUs;
	uint64_t performanceRoundCount;
	uint64_t performanceRoundTotalUs;
	uint64_t performanceRoundMaxUs;
	uint64_t performanceStartSentUs;
	uint64_t performanceStartSendCount;
	uint64_t performanceStartSendTotalUs;
	uint64_t performanceStartSendMaxUs;
	uint64_t performanceSocketDataCount;
	uint64_t performanceSocketDataTotalUs;
	uint64_t performanceSocketDataMaxUs;
	int performanceSocketError;
	char lastError[96];
};

void GBASIONetlinkCreate(struct GBASIONetlink*);
void GBASIONetlinkDestroy(struct GBASIONetlink*);
bool GBASIONetlinkHost(struct GBASIONetlink*, int port);
bool GBASIONetlinkJoin(struct GBASIONetlink*, const char* host, int port);
bool GBASIONetlinkSetTracePath(struct GBASIONetlink*, const char* path);
bool GBASIONetlinkSetDiagnosticPath(struct GBASIONetlink*, const char* path);
bool GBASIONetlinkSetPerformancePath(struct GBASIONetlink*, const char* path, uint64_t (*clockUs)(void));
void GBASIONetlinkRecordFrame(struct GBASIONetlink*);
enum GBASIONetlinkConnectionState GBASIONetlinkGetState(const struct GBASIONetlink*);
const char* GBASIONetlinkGetError(const struct GBASIONetlink*);

CXX_GUARD_END

#endif
