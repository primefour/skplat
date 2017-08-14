/*******************************************************************************
 **      Filename: log/slog.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-28 14:19:36
 ** Last Modified: 2017-03-28 14:19:36
 ******************************************************************************/
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <string>
#include "logbase.h"

class SKTracer{
public:
    SKTracer(LogLevel _level, const char* _tag, const char* _name, 
                    const char* _file, const char* _func, int _line, const char* _log)
    :m_enable(log_for(_level)), m_info(), m_tv() {
    	m_info.level = _level;

        if (m_enable) {
		    m_info.tag = _tag;
		    m_info.filename = _file;
		    m_info.func_name = _func;
		    m_info.line = _line;
	        gettimeofday(&m_info.timeval, NULL);
	        m_info.pid = -1;
	        m_info.tid = -1;
	        m_info.maintid = -1;

	        strncpy(m_name, _name, sizeof(m_name));
	        m_name[sizeof(m_name)-1] = '\0';

            m_tv = m_info.timeval;
            char strout[1024] = {'\0'};
            snprintf(strout, sizeof(strout), "enter>> %s %s", m_name, NULL!=_log? _log:"");
            log_write(&m_info, strout);
        }
    }

    ~SKTracer(){
        if (m_enable) {
            timeval tv;
            gettimeofday(&tv, NULL);
            m_info.timeval = tv;
            long timeSpan = (tv.tv_sec - m_tv.tv_sec) * 1000 + (tv.tv_usec - m_tv.tv_usec) / 1000;
            char strout[1024] = {'\0'};
            if(m_exitmsg.empty()){
                snprintf(strout, sizeof(strout), "exit>> %s  %ld", m_name, timeSpan);
            }else{
                snprintf(strout, sizeof(strout), "exit>> %s  %ld %s", m_name, timeSpan, m_exitmsg.c_str());
            }
            log_write(&m_info, strout);
        }
    }
    
    void Exit(const std::string& _exitmsg){
        m_exitmsg += _exitmsg;
    }
    
private:
    SKTracer(const SKTracer&){};
    SKTracer& operator=(const SKTracer&){return *this;};

private:
    bool m_enable;
    LogInfo m_info;
	char m_name[128];
	timeval m_tv;
    
    std::string m_exitmsg;
};
