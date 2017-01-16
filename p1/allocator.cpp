#include "allocator.h"

std::vector <char *> Pointer::ptrs;

Pointer::Pointer() {}

Pointer::Pointer(char *ptr1, size_t size1, size_t num1)
{
    pos = ptrs.size();
    ptrs.push_back(ptr1);
    size = size1;
    num = num1;
}

char * Pointer::get() const
{
    return ptrs[pos];
}

void Pointer::set(char *ptr1)
{
    ptrs[pos] = ptr1;
}

Allocator::Allocator(void *base1, size_t size1)
{
    base = (char *) base1;
    size = size1;
    counter = 0;
}

Pointer Allocator::alloc(size_t n)
{
    if (base + n <= (ptrs.size() == 0 ? base + size : ptrs[0].get())) {
        Pointer res(base, n, counter++);
        ptrs.insert(ptrs.begin(), res);
        return res;
    }
    if (ptrs.size() > 0 && ptrs.back().get() + ptrs.back().size + n <= base + size) {
        Pointer res(ptrs.back().get() + ptrs.back().size, n, counter++);
        ptrs.push_back(res);
        return res;
    }

    for (size_t i = 1; i < ptrs.size(); i++) {
        if (ptrs[i - 1].get() + ptrs[i - 1].size + n <= ptrs[i].get()) {
            Pointer res(ptrs[i - 1].get() + ptrs[i - 1].size, n, counter++);
            ptrs.insert(ptrs.begin() + i, res);
            return res;
        }
    }
    throw AllocError(AllocErrorType::NoMemory, "No memory for alloc");
}

void Allocator::realloc(Pointer &p, size_t n)
{
    for (size_t i = 0; i < ptrs.size(); i++) {
        if (p.num == ptrs[i].num) {
            if (ptrs[i].get() + n <= (i + 1 == ptrs.size() ? base + size : ptrs[i + 1].get())) {
                ptrs[i].size = n;
                p.size = n;
            } else {
                memcpy(buf, ptrs[i].get(), p.size);
                ptrs.erase(ptrs.begin() + i);
                defrag();

                if (ptrs.back().get() + ptrs.back().size + n > base + size) {
                    throw AllocError(AllocErrorType::NoMemory, "No memory for realloc");
                }
                memcpy(ptrs.back().get() + ptrs.back().size, buf, std::min(p.size, n));
                p = Pointer(ptrs.back().get() + ptrs.back().size, n, p.num);
                ptrs.push_back(p);
            }
            return;
        }
    }
    p = alloc(n);
}

void Allocator::free(Pointer &p)
{
    for (size_t i = 0; i < ptrs.size(); i++) {
        if (p.num == ptrs[i].num) {
            ptrs.erase(ptrs.begin() + i);
            p.set(NULL);
            return;
        }
    }
    throw AllocError(AllocErrorType::InvalidFree, "Free of non-existing pointer");
}

void Allocator::defrag()
{
    if (ptrs.size() == 0) {
        return;
    }
    size_t ind = 0, pos = 0;
    bool flag = 1;
    std::vector <char *> newptrs(ptrs.size(), base);

    for (size_t i = 0; i < size; i++) {
        while (ptrs[ind].get() + ptrs[ind].size <= base + i && ind + 1 < ptrs.size()) {
            ind++;
            flag = 1;
        }

        if (ptrs[ind].get() <= base + i && base + i < ptrs[ind].get() + ptrs[ind].size) {
            if (flag) {
                newptrs[ind] = base + pos;
                flag = 0;
            }
            *(base + pos) = *(base + i);
            pos++;
        }
    }

    for (size_t i = 0; i < ptrs.size(); i++) {
        ptrs[i].set(newptrs[i]);
    }
}
