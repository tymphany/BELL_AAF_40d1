/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*   %%version */

#ifndef HAL_UTILS_H
#define HAL_UTILS_H

#include "hal/hal_registers.h"

/****************************************************************************
FUNCTIONS FOR USE IN MANUALLY WRITTEN CODE

This section contains macros which are intended to be invoked by the
rest of the firmware. These should be used alongside the auto-generated
macros in hal_macros.h.

*/

/**
 * Set a single field in a variable as if it were the given register, so
 * that the variable can later be used to set the register itself.
 * This macro checks that the given field name exists in the given
 * register, and fails to compile if not.
 *
 * If you want to set multiple fields at once in an efficient way, use
 * hal_set_var_for_register_fields.
 *
 * EXAMPLE
 *
 * hal_set_var_for_register_field(tardiness, CLKGEN_MINMAX_MMU_RATE,
 *           CLKGEN_MIN_MMU_RATE, min_rate);
 *
 */
/*lint --emacro((656),hal_set_var_for_register_field) */
/*lint --emacro((834),hal_set_var_for_register_field) */
#define hal_set_var_for_register_field(var, register, field, value)     \
    CHECK_FIELD_TYPE(register, field)                                   \
    ((var) = ((var) & ~FIELD_MASK_SHIFTED(field)) |                     \
     ((value) << FIELD_SHIFT(field)))


/**
 * Set multiple fields in a variable as if it were the given register, so
 * that the variable can later be used to set the register itself.
 *
 * This macro checks that the given field names exist in the given
 * register, and fails to compile if not.
 *
 * The syntax for this is slightly odd, because of the way it abuses the
 * pre-processor to do the job.
 *
 * EXAMPLE
 *
 *  hal_set_var_for_register_fields(varname, REGISTER, (
 *       (FIELDNAME, value,
 *       (FIELDNAME, value,
 *       ...
 *       (FIELDNAME, value, FIELDS_END
 *       )))));
 *
 *  e.g.
 *
 *  hal_set_var_for_register_fields(tardiness, CLKGEN_MINMAX_MMU_RATE,
 *          (CLKGEN_MAX_MMU_RATE, dormpsg.MAX_MMU_CLOCK,
 *           (CLKGEN_MIN_MMU_RATE, dormpsg.MAX_MMU_CLOCK, FIELDS_END)));
 *
 */
/*lint --emacro((656),hal_set_var_for_register_fields) */
/*lint --emacro((834),hal_set_var_for_register_fields) */
#define hal_set_var_for_register_fields(var, register, fields)  \
    CHECK_FIELD_TYPE(register, FIRST_FIELD_NAME fields)         \
    ((var) = ((var) & ~FIELDS_MASKS_SHIFTED(register,fields)) | \
     (FIELDS_VALUES_SHIFTED(register, fields)))


/**
 * Set multiple fields in the same register.
 *
 * This macro checks that the given field names exist in the same
 * register, and fails to compile if not.
 *
 * The syntax for this is slightly odd, because of the way it abuses the
 * pre-processor to do the job.
 *
 * EXAMPLE
 * hal_set_fields(
 *       FIELDNAME, value,
 *       (FIELDNAME, value,
 *         ...
 *       (FIELDNAME, value, FIELDS_END
 *       )));
 *
 *  e.g.
 *
 *  hal_set_fields(INT_CLRB, 1, (INT_EN, 1, (INT_EVENT_EN, 1, FIELDS_END)));
 *
 */
/*lint --emacro((656),hal_set_fields) */
/*lint --emacro((834),hal_set_fields) */
#define hal_set_fields(first, val, rest)                                \
    hal_set_register_fields(REGISTER_FOR(first), (first, val, rest))


/**
 *
 * FUNCTIONS FOR USE IN AUTOGENERATED CODE
 *
 * This section contains macros which are intended to be invoked by the
 * hal_macros.h auto-generated header file. The rest of the firmware should
 * not need to invoke them, because hal_macros.h should define the relevant
 * accessor macro, e.g.
 *
 * #define hal_set_disco_lights_en(x) hal_set_field(DISCO_LIGHTS_EN,(x))
 *
 * It also contains the macros which are invoked by the macros in the
 * previous section.
 */
/*lint --emacro((656),hal_set_field) */
/*lint --emacro((834),hal_set_field) */
#define hal_set_field(field, value)                             \
    hal_set_register_field(REGISTER_FOR(field), field, value)

/*lint --emacro((656),hal_set_register_field) */
/*lint --emacro((834),hal_set_register_field) */
#define hal_set_register_field(register, field, value)          \
    CHECK_FIELD_TYPE(register, field)                           \
    ((register) = ((register) & ~FIELD_MASK_SHIFTED(field)) |   \
     ((value) << FIELD_SHIFT(field)))

/*lint --emacro((656),hal_set_register_fields) */
/*lint --emacro((834),hal_set_register_fields) */
#define hal_set_register_fields(register, fields)                       \
    CHECK_FIELD_TYPE(register, FIRST_FIELD_NAME fields)                 \
    ((register) = ((register) & ~FIELDS_MASKS_SHIFTED(register,fields)) | \
     (FIELDS_VALUES_SHIFTED(register, fields)))

/*lint --emacro((656),hal_get_field) */
/*lint --emacro((834),hal_get_field) */
#define hal_get_field(field)                            \
    hal_get_register_field(REGISTER_FOR(field), field)

/*lint --emacro((656),hal_get_register_field) */
/*lint --emacro((834),hal_get_register_field) */
#define hal_get_register_field(register, field)                 \
    CHECK_FIELD_TYPE(register, field)                           \
    (((register) >> FIELD_SHIFT(field)) & FIELD_MASK(field))

/*lint --emacro((656),hal_get_register_field8) */
/*lint --emacro((834),hal_get_register_field8) */
#define hal_get_register_field8(register, field)                 \
    CHECK_FIELD_TYPE(register, field)                           \
    ((uint8)(((register) >> FIELD_SHIFT(field)) & FIELD_MASK(field)))

/*lint --emacro((656),hal_test_field) */
/*lint --emacro((834),hal_test_field) */
#define hal_test_field(field)                           \
    hal_test_register_field(REGISTER_FOR(field), field)

/*lint --emacro((656),hal_test_register_field) */
/*lint --emacro((834),hal_test_register_field) */
#define hal_test_register_field(register, field)        \
    CHECK_FIELD_TYPE(register, field)                   \
    ((register) & FIELD_MASK_SHIFTED(field))

/****************************************************************************
HIDDEN WORKINGS

This section contains stuff from which you should avert your eyes. Nothing to
see here, move along.

*/

#define FIELDS_END (END, 0, (END))

/* We enforce that the bits for reg REG are named REG_..._[ML]SB_POSN */

#define REGISTER_FOR(bit) REGISTER_FOR_(bit)
#define REGISTER_FOR_(bit) REGISTER_FOR_ ## bit
#define TYPE_OF(bit) TYPE_OF_(bit)
#define TYPE_OF_(bit) TYPE_OF_ ## bit

#define FIELD_MASK(field) FIELD_MASK_1(REGISTER_FOR(field), field)
#define FIELD_MASK_1(reg, field) FIELD_MASK_2(reg, field)
#define FIELD_MASK_2(reg, field)                                \
    FIELD_MASK_3(reg ## _ ## field ## _MSB_POSN,                \
                 reg ## _ ## field ## _LSB_POSN)
#define FIELD_MASK_3(msb, lsb)                                          \
    (((msb) >= (lsb)) ? (uint16)((1uL << (((msb) - (lsb)) + 1)) - 1u) : 0u)
#define FIELD_SHIFT(field) FIELD_SHIFT_1(REGISTER_FOR(field), field)
#define FIELD_SHIFT_1(reg, field) FIELD_SHIFT_2(reg, field)
#define FIELD_SHIFT_2(reg, field) (reg ## _ ## field ## _LSB_POSN)
#define FIELD_MASK_SHIFTED(field) (FIELD_MASK(field) << FIELD_SHIFT(field))

#define FIELDS_MASKS_SHIFTED(register, fields) \
    (FIELDS_MASKS_SHIFTED_ ## register fields)

#define FIELDS_VALUES_SHIFTED(register, fields) \
    (FIELDS_VALUES_SHIFTED_ ## register fields)

#define FIELDS_MASKS_SHIFTED_END(end) 0u
#define FIELDS_VALUES_SHIFTED_END(end) 0u

#define FIRST_FIELD_NAME(a, b, c) a
#define TYPE_OF_END END
#define REGISTER_FOR_END_END_LSB_POSN ( 0)
#define REGISTER_FOR_END_END_MSB_POSN (/*lint --e(504) */-1)

#define CHECK_FIELD_TYPE(reg, bit) CHECK_FIELD_TYPE_(reg, TYPE_OF(bit))
#define CHECK_FIELD_TYPE_(reg, type) CHECK_FIELD_TYPE__(reg, type)
#define CHECK_FIELD_TYPE__(reg, type) \
    CHECK_FIELD_TYPE_ ## reg ## _ ## type

#endif /* HAL_UTILS_H */
