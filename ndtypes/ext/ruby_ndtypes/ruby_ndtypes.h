/* File containing headers for Ruby ndtypes wrapper. 
 *
 * Author: Sameer Deshmukh (@v0dro)
*/

#ifndef RUBY_NDTYPES_H
#define RUBY_NDTYPES_H

#if defined(__cplusplus)
extern "C" {
} /* satisfy cc-mode */
#endif

#include "ruby.h"
#include "ndtypes.h"

/* Public interface for ndtypes. */
typedef struct NdtObject NdtObject;
extern VALUE cNDTypes;

/**
 * @brief    Return 1 if obj is of type NDTypes. 0 otherwise.
 *
 * @param    obj Ruby object whose type has to be checked.
 *
 * @return   Boolean value whether it is NDT object or not.
 */

int rb_ndtypes_check_type(VALUE obj);

/**
 * @brief    Get a pointer to the NdtObject struct that obj contains.
 *
 * @details  This function unwraps the Ruby object obj passed to it and
 * returns a pointer to the `NdtObject` struct that is contained within it.
 * You must passed an NDT Ruby object.
 *
 * @param    obj Ruby object of type NDT.
 *
 * @return   Pointer to the NdtObject encapsulated within the Ruby object.
 */
NdtObject * rb_ndtypes_get_ndt_object(VALUE obj);

/**
 * @brief    Calls TypedData_Make_Struct and returns a Ruby object wrapping over ndt_p.
 *
 * @details  This function calls the TypedData_Make_Struct macro from the Ruby
 * C API for wrapping over argument passed to it. The ndt_p pointer must be allocated
 * and data assingned to its contents before calling this function.
 *
 * @param    ndt_p Pointer to NdtObject allocated by the caller.
 *
 * @return   Ruby object wrapping over NdtObject.
 */
VALUE rb_ndtypes_make_ndt_object(NdtObject *ndt_p);

/**
 * @brief    Perform allocation using TypedData_Wrap_Struct and return a Ruby NDT object.
 *
 * @details  This function does not set any of the data within the struct and assigns
 * the NdtObject struct within the Ruby object to NULL. For allocating an already allocated
 * NdtObject within a Ruby object use the rb_ndtypes_make_ndt_object() function.
 *
 * @return   NDT Ruby object.
 */
VALUE rb_ndtypes_wrap_ndt_object(void);

/**
 * @brief    Unwrap the Ruby object ndt and return the pointer to ndt_t placed within it.
 *
 * @param    ndt Ruby NDT object.
 *
 * @return   Pointer to ndt_t type wrapped within the passed Ruby object.
 */
const ndt_t * rb_ndtypes_const_ndt(VALUE ndt);

/**
 * @brief    Function for taking a source type and moving it across the subtree.
 *
 * @param    src NDTypes Ruby object of the source XND object.
 * @param    t Pointer to type of the view of XND object.
 *
 * @return   NDT Ruby object of the moved subtree.
 */
VALUE rb_ndtypes_move_subtree(VALUE src, ndt_t *t);

/**
 * @brief    Create NDT object from Ruby String. Returns the same object if type is NDT.
 * If you pass a String as an argument, read the string and create an NDT from it.
 *
 * @param    type Ruby object. Can be Ruby String or NDT object.
 *
 * @return   Ruby object of type NDT.
 */
VALUE rb_ndtypes_from_object(VALUE type);

/**
 * @brief    Make Ruby aware of the error condition raised by libndtypes by using
 * the set_error_info() Ruby C API function.
 *
 * @param    ctx Struct of type ndt_context_t.
 *
 * @return   Ruby error class object.
 */

VALUE rb_ndtypes_set_error(ndt_context_t *ctx);

/**
 * @brief    Create an NDT Ruby object from an ndt_t struct.
 *
 * @details  The caller can use this function for creating a Ruby NDT
 * object from a ndt_t struct that has already been allocated using one
 * of the allocation functions from libndtypes like ndt_from_string().
 *
 * @param    type An allocated pointer to a struct of type ndt_t.
 *
 * @return   Ruby object of type NDT.
 */
VALUE rb_ndtypes_from_type(const ndt_t *type);

#define INT2BOOL(t) (t ? Qtrue : Qfalse)

#if defined(__cplusplus)
} /* extern "C" { */
#endif

#endif  /* RUBY_NDTYPES_H */
