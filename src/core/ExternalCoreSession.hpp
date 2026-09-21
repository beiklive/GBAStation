#pragma once

#include <string>

namespace beiklive
{
    /// Whether the launcher should track this platform's play stats itself.
    ///
    /// The launcher normally opens a session before chainloading an external
    /// core: it bumps playCount/lastPlayed up front and later adds playTime when
    /// the core returns with "--external-return <token>".  Saturn
    /// (YabaSanshiro) maintains those fields from inside the core instead, so
    /// starting a session for it would double count playCount and leave a stale
    /// external_core_session.json behind.
    bool platformReportsOwnStats(int platform);

    std::string makeExternalCoreSessionToken(const std::string& romPath);
    bool beginExternalCoreSession(const std::string& romPath, int platform,
                                  const std::string& token);
    bool finishExternalCoreSession(const std::string& token);
    std::string externalCoreReturnToken(int argc, char* argv[]);
}
