#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>

enum {MAXN = (1 << 18)};

enum class AllocErrorType
{
    InvalidFree,
    NoMemory,
};

class AllocError: std::runtime_error
{
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
        runtime_error(message),
        type(_type)
    {}

    AllocErrorType getType() const
    {
        return type;
    }
};

class Allocator;

class Pointer
{
    friend class Allocator;

    static std::vector <char *> ptrs;
    size_t pos;
    size_t num;
    size_t size;

public:
    Pointer();
    Pointer(char *ptr1, size_t size1, size_t num1);

    char * get() const;
    void set(char *);
};

class Allocator
{
    char *base;
    size_t size;
    size_t counter;
    std::vector <Pointer> ptrs;
    char buf[MAXN];

public:
    Allocator(void *base1, size_t size1);
    
    Pointer alloc(size_t n);
    void realloc(Pointer &p, size_t n);
    void free(Pointer &p);
    void defrag();
};
