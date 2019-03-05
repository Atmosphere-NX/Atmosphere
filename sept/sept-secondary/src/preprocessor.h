/* TuxSH: I added INC/DEC_10 to INC/DEC_32; tuples */

#ifndef FUSEE_PREPROCESSOR_H
#define FUSEE_PREPROCESSOR_H

/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    cloak.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

/*#ifndef CLOAK_GUARD_H
#define CLOAK_GUARD_H*/

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define COMPL(b) PRIMITIVE_CAT(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0

#define BITAND(x) PRIMITIVE_CAT(BITAND_, x)
#define BITAND_0(y) 0
#define BITAND_1(y) y

#define INC(x) PRIMITIVE_CAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9
#define INC_9 10
#define INC_10 11
#define INC_11 12
#define INC_12 13
#define INC_13 14
#define INC_14 15
#define INC_15 16
#define INC_16 17
#define INC_17 18
#define INC_18 19
#define INC_19 20
#define INC_20 21
#define INC_21 22
#define INC_22 23
#define INC_23 24
#define INC_24 25
#define INC_25 26
#define INC_26 27
#define INC_27 28
#define INC_28 29
#define INC_29 30
#define INC_30 31
#define INC_31 32
#define INC_32 32
#define INC_33 33

#define DEC(x) PRIMITIVE_CAT(DEC_, x)
#define DEC_0 0
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8
#define DEC_10 9
#define DEC_11 10
#define DEC_12 11
#define DEC_13 12
#define DEC_14 13
#define DEC_15 14
#define DEC_16 15
#define DEC_17 16
#define DEC_18 17
#define DEC_19 18
#define DEC_20 19
#define DEC_21 20
#define DEC_22 21
#define DEC_23 22
#define DEC_24 23
#define DEC_25 24
#define DEC_26 25
#define DEC_27 26
#define DEC_28 27
#define DEC_29 28
#define DEC_30 29
#define DEC_31 30
#define DEC_32 31
#define DEC_33 32

#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0,)
#define PROBE(x) x, 1,

#define IS_PAREN(x) CHECK(IS_PAREN_PROBE x)
#define IS_PAREN_PROBE(...) PROBE(~)

#define NOT(x) CHECK(PRIMITIVE_CAT(NOT_, x))
#define NOT_0 PROBE(~)

#define COMPL(b) PRIMITIVE_CAT(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0

#define BOOL(x) COMPL(NOT(x))

#define IIF(c) PRIMITIVE_CAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t

#define IF(c) IIF(BOOL(c))

#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(id) id DEFER(EMPTY)()

#define EVAL(...)  EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define REPEAT(count, macro, ...) \
    WHEN(count) \
    ( \
        OBSTRUCT(REPEAT_INDIRECT) () \
        ( \
            DEC(count), macro, __VA_ARGS__ \
        ) \
        OBSTRUCT(macro) \
        ( \
            DEC(count), __VA_ARGS__ \
        ) \
    )
#define REPEAT_INDIRECT() REPEAT

#define WHILE(pred, op, ...) \
    IF(pred(__VA_ARGS__)) \
    ( \
        OBSTRUCT(WHILE_INDIRECT) () \
        ( \
            pred, op, op(__VA_ARGS__) \
        ), \
        __VA_ARGS__ \
    )
#define WHILE_INDIRECT() WHILE

#define PRIMITIVE_COMPARE(x, y) IS_PAREN \
( \
    COMPARE_ ## x ( COMPARE_ ## y) (())  \
)

#define IS_COMPARABLE(x) IS_PAREN( CAT(COMPARE_, x) (()) )

#define NOT_EQUAL(x, y) \
IIF(BITAND(IS_COMPARABLE(x))(IS_COMPARABLE(y)) ) \
( \
   PRIMITIVE_COMPARE, \
   1 EAT \
)(x, y)

#define EQUAL(x, y) COMPL(NOT_EQUAL(x, y))

#define COMMA() ,

#define COMMA_IF(n) IF(n)(COMMA, EAT)()

#define PLUS() +

#define _TUPLE_ELEM_0(a, ...) a
#define _TUPLE_ELEM_1(a, b, ...) b
#define _TUPLE_ELEM_2(a, b, c, ...) c
#define _TUPLE_ELEM_3(a, b, c, d, ...) d
#define _TUPLE_ELEM_4(a, b, c, d, e, ...) e

#define TUPLE_ELEM_0(T) EVAL(_TUPLE_ELEM_0 T)
#define TUPLE_ELEM_1(T) EVAL(_TUPLE_ELEM_1 T)
#define TUPLE_ELEM_2(T) EVAL(_TUPLE_ELEM_2 T)
#define TUPLE_ELEM_3(T) EVAL(_TUPLE_ELEM_3 T)
#define TUPLE_ELEM_4(T) EVAL(_TUPLE_ELEM_4 T)

#define _TUPLE_FOLD_LEFT_0(i, T, op) (_TUPLE_ELEM_0 CAT(T,i)) op()
#define _TUPLE_FOLD_LEFT_1(i, T, op) (_TUPLE_ELEM_1 CAT(T,i)) op()
#define _TUPLE_FOLD_LEFT_2(i, T, op) (_TUPLE_ELEM_2 CAT(T,i)) op()
#define _TUPLE_FOLD_LEFT_3(i, T, op) (_TUPLE_ELEM_3 CAT(T,i)) op()
#define _TUPLE_FOLD_LEFT_4(i, T, op) (_TUPLE_ELEM_4 CAT(T,i)) op()

#define TUPLE_FOLD_LEFT_0(len, T, op) EVAL(REPEAT(len, _TUPLE_FOLD_LEFT_0, T, op))
#define TUPLE_FOLD_LEFT_1(len, T, op) EVAL(REPEAT(len, _TUPLE_FOLD_LEFT_1, T, op))
#define TUPLE_FOLD_LEFT_2(len, T, op) EVAL(REPEAT(len, _TUPLE_FOLD_LEFT_2, T, op))
#define TUPLE_FOLD_LEFT_3(len, T, op) EVAL(REPEAT(len, _TUPLE_FOLD_LEFT_3, T, op))
#define TUPLE_FOLD_LEFT_4(len, T, op) EVAL(REPEAT(len, _TUPLE_FOLD_LEFT_4, T, op))

#endif
