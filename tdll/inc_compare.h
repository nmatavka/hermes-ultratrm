#ifndef INC_COMPARE_H
#define INC_COMPARE_H

#define INC_NV_COMPARE_DEFINITION(type) \
public: \
    int compare(const type &other) const; \
    bool operator==(const type &other) const { return compare(other) == 0; } \
    bool operator!=(const type &other) const { return compare(other) != 0; } \
    bool operator<(const type &other) const { return compare(other) < 0; } \
    bool operator>(const type &other) const { return compare(other) > 0; } \
    bool operator<=(const type &other) const { return compare(other) <= 0; } \
    bool operator>=(const type &other) const { return compare(other) >= 0; }

#define INC_NV_COMPARE_IMPLEMENTATION(type)

#endif
