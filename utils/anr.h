#ifndef __ANR_H__
#define __ANR_H__

class scope_anr {
  public:
    scope_anr(const char* _file = "", const char* _func = "", int _line = 0);
    ~scope_anr();

    void anr(int _timeout = 15 * 60 * 1000);

  private:
    scope_anr(const scope_anr&);
    scope_anr& operator=(const scope_anr&);

  private:
    const char* file_;
    const char* func_;
    int line_;
};

#define SCOPE_ANR_AUTO(...) scope_anr __anr__var__anonymous_variable__(__FILE__, __func__, __LINE__); __anr__var__anonymous_variable__.anr(__VA_ARGS__)
#define SCOPE_ANR_OBJ(objname) scope_anr objname(__FILE__, __func__, __LINE__)

#endif
