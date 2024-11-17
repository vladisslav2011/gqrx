/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 vladisslav2011@gmail.com.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_TAGGED_UNION_H
#define INCLUDED_TAGGED_UNION_H

#include <string>
#include <iostream>
#include <cstdint>

enum value_type
{
    V_INT=0,
    V_DOUBLE,
    V_STRING,
    V_BOOLEAN
};

struct tag_union_const
{
    enum tag_type
    {
        TAG_INT=0,
        TAG_DOUBLE,
        TAG_STRING
    };
    tag_type tag;
    union union_type {
        int64_t i;
        double d;
        const char * s;
        constexpr union_type(int64_t init):i(init){}
        constexpr union_type(double init):d(init){}
        constexpr union_type(const char * init):s(init){}
    } value;
    constexpr tag_union_const():tag(TAG_INT),value(int64_t(0)){}
    constexpr tag_union_const(const char *init):tag(TAG_STRING),value(init){}
    constexpr tag_union_const(const float &init):tag(TAG_DOUBLE),value(double(init)){}
    constexpr tag_union_const(const double &init):tag(TAG_DOUBLE),value(init){}
    constexpr tag_union_const(const int &init):tag(TAG_INT),value(int64_t(init)){}
    constexpr tag_union_const(const long int &init):tag(TAG_INT),value(int64_t(init)){}
    constexpr tag_union_const(const long long int &init):tag(TAG_INT),value(int64_t(init)){}
    constexpr tag_union_const(const unsigned &init):tag(TAG_INT),value(int64_t(init)){}
    constexpr tag_union_const(const bool &init):tag(TAG_INT),value(int64_t(init)){}
    constexpr void typecheck(const tag_type required, const char * msg) const
    {
        if(tag!=required)
        {
            dprint(msg);
            //throw does not work as expected here
            //emit a message and exit
            exit(1);
        }
    }
    constexpr operator int() const
    {
        typecheck(TAG_INT, "invalid cast to int");
        return value.i;
    }
    constexpr operator long int() const
    {
        typecheck(TAG_INT, "invalid cast to int");
        return value.i;
    }
    constexpr operator long long int() const
    {
        typecheck(TAG_INT, "invalid cast to int");
        return value.i;
    }
    constexpr operator bool() const
    {
        typecheck(TAG_INT, "invalid cast to bool");
        return value.i;
    }
    constexpr operator double() const
    {
        typecheck(TAG_DOUBLE, "invalid cast to double");
        return value.d;
    }
    constexpr operator float() const
    {
        typecheck(TAG_DOUBLE, "invalid cast to float");
        return value.d;
    }
    constexpr operator const char *() const
    {
        typecheck(TAG_STRING, "invalid cast to string");
        return value.s;
    }
    constexpr void dprint(const char * x="") const
    {
        switch(tag)
        {
        case TAG_INT:
            std::cerr<<"v_union:i="<<value.i<<" "<<x<<"\n";
            break;
        case TAG_DOUBLE:
            std::cerr<<"v_union:d="<<value.d<<" "<<x<<"\n";
            break;
        case TAG_STRING:
            std::cerr<<"v_union:s="<<value.s<<" "<<x<<"\n";
            break;
        }
    }
    constexpr bool typecheck_cmp(const tag_union_const & other) const
    {
        if(tag!=other.tag)
        {
            dprint("invalid comparison");
            other.dprint("invalid comparison");
            return false;
        }
        return true;
    }
    constexpr bool operator < (const tag_union_const & other) const
    {
        if(!typecheck_cmp(other))
            return false;
        switch(tag)
        {
        case TAG_INT:
            return value.i<other.value.i;
            break;
        case TAG_DOUBLE:
            return value.d<other.value.d;
            break;
        case TAG_STRING:
            return value.s<other.value.s;
            break;
        }
        return false;
    }
    constexpr bool operator > (const tag_union_const & other) const
    {
        if(!typecheck_cmp(other))
            return false;
        switch(tag)
        {
        case TAG_INT:
            return value.i>other.value.i;
            break;
        case TAG_DOUBLE:
            return value.d>other.value.d;
            break;
        case TAG_STRING:
            return value.s>other.value.s;
            break;
        }
        return false;
    }
    constexpr bool operator == (const tag_union_const & other) const
    {
        if(!typecheck_cmp(other))
            return false;
        switch(tag)
        {
        case TAG_INT:
            return value.i==other.value.i;
            break;
        case TAG_DOUBLE:
            return value.d==other.value.d;
            break;
        case TAG_STRING:
            return value.s==other.value.s;
            break;
        }
        return false;
    }
};

class tag_union
{
    enum tag_type
    {
        TAG_INT=0,
        TAG_DOUBLE,
        TAG_STRING
    };
    tag_type tag;
    union union_type {
        int64_t i;
        double d;
        std::string s;
        union_type():i(0){}
        ~union_type(){}
    } value;
    public:
    tag_union():tag(TAG_INT)
    {
        value.i=0;
    }
    tag_union(const tag_union_const& other):tag(tag_type(other.tag))
    {
        switch(tag)
        {
        case TAG_STRING:
            new (&value.s) std::string(other.value.s);
            break;
        case TAG_DOUBLE:
            value.d=other.value.d;
            break;
        case TAG_INT:
            value.i=other.value.i;
            break;
        }
    }
    tag_union(const tag_union& other):tag(other.tag)
    {
        switch(tag)
        {
        case TAG_STRING:
            new (&value.s) std::string(other.value.s);
            break;
        case TAG_DOUBLE:
            value.d=other.value.d;
            break;
        case TAG_INT:
            value.i=other.value.i;
            break;
        }
    }
    tag_union(const char *init):tag(TAG_STRING)
    {
        new (&value.s) std::string(init);
    }
    tag_union(const std::string &init):tag(TAG_STRING)
    {
        new (&value.s) std::string(init);
    }
    tag_union(const float &init):tag(TAG_DOUBLE)
    {
        value.d = (double)init;
    }
    tag_union(const double &init):tag(TAG_DOUBLE)
    {
        value.d = init;
    }
    tag_union(const int &init):tag(TAG_INT)
    {
        value.i = init;
    }
    tag_union(const long int &init):tag(TAG_INT)
    {
        value.i = init;
    }
    tag_union(const long long int &init):tag(TAG_INT)
    {
        value.i = init;
    }
    tag_union(const bool &init):tag(TAG_INT)
    {
        value.i = init;
    }
    tag_union(const value_type vt, const std::string & from)
    {
        switch(vt)
        {
        case V_INT:
                value.i=stoll(from);
                tag=TAG_INT;
            break;
        case V_DOUBLE:
                value.d=stod(from);
                tag=TAG_DOUBLE;
            break;
        case V_STRING:
                new (&value.s) std::string(from);
                tag=TAG_STRING;
            break;
        case V_BOOLEAN:
                value.i=(from=="true");
                tag=TAG_INT;
            break;
        }
    }
    ~tag_union()
    {
        if(tag==TAG_STRING)
            value.s.~basic_string();
    }
    void typecheck(const tag_type required, const char * msg) const
    {
        if(tag!=required)
        {
            dprint(msg);
            //throw does not work as expected here
            //emit a message and exit
            exit(1);
        }
    }
    operator int() const
    {
        typecheck(TAG_INT, "invalid cast to int");
        return value.i;
    }
    operator long int() const
    {
        typecheck(TAG_INT, "invalid cast to int");
        return value.i;
    }
    operator long long int() const
    {
        typecheck(TAG_INT, "invalid cast to int");
        return value.i;
    }
    operator bool() const
    {
        typecheck(TAG_INT, "invalid cast to bool");
        return value.i;
    }
    operator double() const
    {
        typecheck(TAG_DOUBLE, "invalid cast to double");
        return value.d;
    }
    operator float() const
    {
        typecheck(TAG_DOUBLE, "invalid cast to float");
        return value.d;
    }
    operator std::string() const
    {
        typecheck(TAG_STRING, "invalid cast to string");
        return value.s;
    }
    const std::string to_string() const
    {
        switch(tag)
        {
        case TAG_INT:
            return std::to_string(value.i);
        case TAG_DOUBLE:
            return std::to_string(value.d);
        case TAG_STRING:
            return value.s;
        default:
            return "";
        }
    }
    void dprint(const char * x="") const
    {
        switch(tag)
        {
        case TAG_INT:
            std::cerr<<"tag_union:i="<<value.i<<" "<<x<<"\n";
            break;
        case TAG_DOUBLE:
            std::cerr<<"tag_union:d="<<value.d<<" "<<x<<"\n";
            break;
        case TAG_STRING:
            std::cerr<<"tag_union:s="<<value.s<<" "<<x<<"\n";
            break;
        }
    }
    bool typecheck_cmp(const tag_union & other) const
    {
        if(tag!=other.tag)
        {
            dprint("invalid comparison");
            other.dprint("invalid comparison");
            return false;
        }
        return true;
    }
    bool operator < (const tag_union & other) const
    {
        if(!typecheck_cmp(other))
            return false;
        switch(tag)
        {
        case TAG_INT:
            return value.i<other.value.i;
            break;
        case TAG_DOUBLE:
            return value.d<other.value.d;
            break;
        case TAG_STRING:
            return value.s<other.value.s;
            break;
        }
        return false;
    }
    bool operator > (const tag_union & other) const
    {
        if(!typecheck_cmp(other))
            return false;
        switch(tag)
        {
        case TAG_INT:
            return value.i>other.value.i;
            break;
        case TAG_DOUBLE:
            return value.d>other.value.d;
            break;
        case TAG_STRING:
            return value.s>other.value.s;
            break;
        }
        return false;
    }
    bool operator == (const tag_union & other) const
    {
        if(!typecheck_cmp(other))
            return false;
        switch(tag)
        {
        case TAG_INT:
            return value.i==other.value.i;
            break;
        case TAG_DOUBLE:
            return value.d==other.value.d;
            break;
        case TAG_STRING:
            return value.s==other.value.s;
            break;
        }
        return false;
    }
    bool operator != (const tag_union & other) const
    {
        return !(*this == other);
    }
    tag_union& operator=(const tag_union& other)
    {
        if(tag == TAG_STRING)
        {
            if(other.tag != TAG_STRING)
                value.s.~basic_string();
            else
                value.s=other.value.s;
        }
        if(tag != TAG_STRING && other.tag == TAG_STRING)
            new (&value.s) std::string(other.value.s);
        else{
            switch(other.tag)
            {
            case TAG_INT:
                value.i=other.value.i;
                break;
            case TAG_DOUBLE:
                value.d=other.value.d;
                break;
            case TAG_STRING:
                break;
            }
        }
        tag=other.tag;
        return *this;
    }
#if 0
    tag_union& operator=(const tag_union_const& other)
    {
        if(tag == TAG_STRING)
        {
            if(tag_type(other.tag) != TAG_STRING)
                value.s.~basic_string();
            else
                value.s=std::string(other.value.s);
        }
        if(tag != TAG_STRING && tag_type(other.tag) == TAG_STRING)
            new (&value.s) std::string(other.value.s);
        else{
            switch(tag_type(other.tag))
            {
            case TAG_INT:
                value.i=other.value.i;
                break;
            case TAG_DOUBLE:
                value.d=other.value.d;
                break;
            case TAG_STRING:
                break;
            }
        }
        tag=tag_type(other.tag);
        return *this;
    }
#endif
};
#endif