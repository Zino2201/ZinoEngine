#pragma once

#include "EngineCore.h"
#include "delegates/Delegate.h"

namespace ze::app
{

/**
 * Base abstract class for applications
 * Provides a main loop to operate on
 */
class CORE_API App
{
public:
	App(const int& argc, const char** argv);
	virtual ~App() = default;

	/** Run the main loop, will return only when Exit is called */
	int run();
	void exit(const int& err_code);

	/** Process all pending events, called by run() loop by default but can be called if needed */
	virtual void process_events() = 0;

	ZE_FORCEINLINE static App* get() { return current_app; }
protected:
	virtual void loop() = 0;
private:
	inline static App* current_app = nullptr;
	int err_code;
protected:
	bool _run;
};

CORE_API void exit(const int& err_code);

}