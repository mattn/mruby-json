#include <mruby.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/numeric.h>
#include <mruby/variable.h>
#include <mruby/version.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "parson.h"

#ifdef MRB_WITHOUT_FLOAT
#ifndef JSON_FIXED_NUMBER
#error "Without float, fixed number must be turned on!"
#endif
#endif

#if 1
#define ARENA_SAVE \
  int ai = mrb_gc_arena_save(mrb); \
  if (ai == MRB_ARENA_SIZE) { \
    mrb_raise(mrb, E_RUNTIME_ERROR, "arena overflow"); \
  }
#define ARENA_RESTORE \
  mrb_gc_arena_restore(mrb, ai);
#else
#define ARENA_SAVE
#define ARENA_RESTORE
#endif

#define E_PARSER_ERROR mrb_class_get_under(mrb, mrb_module_get(mrb, "JSON"), "ParserError")
#define E_GENERATOR_ERROR mrb_class_get_under(mrb, mrb_module_get(mrb, "JSON"), "GeneratorError")
#define E_NESTING_ERROR mrb_class_get_under(mrb, mrb_module_get(mrb, "JSON"), "NestingError")

/* Deepest nesting generate follows; a structure that holds itself would
 * otherwise recurse until the C stack runs out. */
#define MAX_NESTING 2048

/*********************************************************
 * main
 *********************************************************/
static void
pretty_cat(mrb_state* mrb, mrb_value str, int pretty) {
  int i;
  mrb_str_cat_lit(mrb, str, "\n");
  for (i = 0; i < pretty; i++) mrb_str_cat_lit(mrb, str, "  ");
}

/* Appends value to str as a JSON string. Runs of bytes that need no escape
 * are copied in one call. */
static void
json_cat_string(mrb_state* mrb, mrb_value str, mrb_value value) {
  const char* ptr = RSTRING_PTR(value);
  const char* end = RSTRING_END(value);
  const char* run = ptr;
  char buf[7];

  mrb_str_cat_lit(mrb, str, "\"");
  while (ptr < end) {
    unsigned char c = (unsigned char)*ptr;
    const char* esc = NULL;
    switch (c) {
    case '\\': esc = "\\\\"; break;
    case '"':  esc = "\\\""; break;
    case '\b': esc = "\\b"; break;
    case '\f': esc = "\\f"; break;
    case '\n': esc = "\\n"; break;
    case '\r': esc = "\\r"; break;
    case '\t': esc = "\\t"; break;
    default:
      if (c < 0x20) {
        snprintf(buf, sizeof(buf), "\\u%04x", c);
        esc = buf;
      }
    }
    if (esc) {
      if (run < ptr) mrb_str_cat(mrb, str, run, ptr - run);
      mrb_str_cat_cstr(mrb, str, esc);
      run = ptr + 1;
    }
    ptr++;
  }
  if (run < end) mrb_str_cat(mrb, str, run, end - run);
  mrb_str_cat_lit(mrb, str, "\"");
}

/* Whether json_cat_value writes value itself rather than asking its to_json */
static mrb_bool
json_builtin_p(mrb_value value) {
  if (mrb_nil_p(value)) return TRUE;
  switch (mrb_type(value)) {
  case MRB_TT_FIXNUM:
#ifndef MRB_WITHOUT_FLOAT
  case MRB_TT_FLOAT:
#endif
  case MRB_TT_TRUE:
  case MRB_TT_FALSE:
  case MRB_TT_UNDEF:
  case MRB_TT_SYMBOL:
  case MRB_TT_STRING:
  case MRB_TT_HASH:
  case MRB_TT_ARRAY:
    return TRUE;
  default:
    return FALSE;
  }
}

/* Appends value to str as JSON. Nested values are written into the same
 * buffer instead of being built as strings of their own and then copied. */
static void
json_cat_value(mrb_state* mrb, mrb_value str, mrb_value value, int pretty, int depth) {
  if (mrb_nil_p(value)) {
    mrb_str_cat_lit(mrb, str, "null");
    return;
  }

  switch (mrb_type(value)) {
#ifndef MRB_WITHOUT_FLOAT
  case MRB_TT_FLOAT:
    {
      mrb_float f = mrb_float(value);
      if (isnan(f) || isinf(f)) {
        mrb_raise(mrb, E_GENERATOR_ERROR, isnan(f) ? "NaN not allowed in JSON" : "Infinity not allowed in JSON");
      }
    }
    mrb_str_concat(mrb, str, mrb_funcall(mrb, value, "to_s", 0, NULL));
    break;
#endif
  case MRB_TT_FIXNUM:
  case MRB_TT_TRUE:
  case MRB_TT_FALSE:
  case MRB_TT_UNDEF:
    mrb_str_concat(mrb, str, mrb_funcall(mrb, value, "to_s", 0, NULL));
    break;
  case MRB_TT_SYMBOL:
    json_cat_string(mrb, str, mrb_funcall(mrb, value, "to_s", 0, NULL));
    break;
  case MRB_TT_STRING:
    json_cat_string(mrb, str, value);
    break;
  case MRB_TT_HASH:
    {
      mrb_value keys;
      mrb_int n, l;

      if (depth >= MAX_NESTING) {
        mrb_raise(mrb, E_NESTING_ERROR, "nesting too deep");
      }
      mrb_str_cat_lit(mrb, str, "{");
      keys = mrb_hash_keys(mrb, value);
      l = RARRAY_LEN(keys);
      if (l == 0) {
        if (pretty >= 0) mrb_str_cat_lit(mrb, str, "\n");
        mrb_str_cat_lit(mrb, str, "}");
        break;
      }
      if (pretty >= 0) pretty_cat(mrb, str, ++pretty);
      for (n = 0; n < l; n++) {
        int ai = mrb_gc_arena_save(mrb);
        mrb_value key = mrb_ary_entry(keys, n);
        if (!mrb_string_p(key)) key = mrb_funcall(mrb, key, "to_s", 0, NULL);
        json_cat_string(mrb, str, key);
        mrb_str_cat_lit(mrb, str, ":");
        json_cat_value(mrb, str, mrb_hash_get(mrb, value, mrb_ary_entry(keys, n)), pretty, depth + 1);
        if (n != l - 1) {
          mrb_str_cat_lit(mrb, str, ",");
          if (pretty >= 0) pretty_cat(mrb, str, pretty);
        }
        mrb_gc_arena_restore(mrb, ai);
      }
      if (pretty >= 0) pretty_cat(mrb, str, --pretty);
      mrb_str_cat_lit(mrb, str, "}");
      break;
    }
  case MRB_TT_ARRAY:
    {
      mrb_int n, l;

      if (depth >= MAX_NESTING) {
        mrb_raise(mrb, E_NESTING_ERROR, "nesting too deep");
      }
      mrb_str_cat_lit(mrb, str, "[");
      l = RARRAY_LEN(value);
      if (l == 0) {
        if (pretty >= 0) mrb_str_cat_lit(mrb, str, "\n");
        mrb_str_cat_lit(mrb, str, "]");
        break;
      }
      if (pretty >= 0) pretty_cat(mrb, str, ++pretty);
      for (n = 0; n < l; n++) {
        int ai = mrb_gc_arena_save(mrb);
        json_cat_value(mrb, str, mrb_ary_entry(value, n), pretty, depth + 1);
        if (n != l - 1) {
          mrb_str_cat_lit(mrb, str, ",");
          if (pretty >= 0) pretty_cat(mrb, str, pretty);
        }
        mrb_gc_arena_restore(mrb, ai);
      }
      if (pretty >= 0) pretty_cat(mrb, str, --pretty);
      mrb_str_cat_lit(mrb, str, "]");
      break;
    }
  default:
    /* Object#to_json writes to_s as a string unless a class overrides it */
    mrb_str_concat(mrb, str, mrb_obj_as_string(mrb, mrb_funcall(mrb, value, "to_json", 0, NULL)));
  }
}

static mrb_value
mrb_value_to_string(mrb_state* mrb, mrb_value value, int pretty) {
  mrb_value str = mrb_str_new_capa(mrb, 64);
  json_cat_value(mrb, str, value, pretty, 0);
  return str;
}

#ifdef JSON_FIXED_NUMBER
/* An integer that does not fit in mrb_int becomes a Float */
static mrb_value
json_int_value(mrb_state* mrb, intmax_t i) {
  if ((intmax_t)MRB_INT_MIN <= i && i <= (intmax_t)MRB_INT_MAX) {
#if MRUBY_RELEASE_MAJOR >= 3
    return mrb_int_value(mrb, (mrb_int)i);
#else
    if (FIXABLE(i)) return mrb_fixnum_value((mrb_int)i);
#endif
  }
#ifndef MRB_WITHOUT_FLOAT
  return mrb_float_value(mrb, (mrb_float)i);
#else
  mrb_raise(mrb, E_ARGUMENT_ERROR, "integer out of range in non-float environment!");
  return mrb_nil_value();
#endif
}
#endif

static mrb_value
json_value_to_mrb_value(mrb_state* mrb, JSON_Value* value) {
  mrb_value ret;
  switch (json_value_get_type(value)) {
  case JSONError:
  case JSONNull:
    ret = mrb_nil_value();
    break;
  case JSONString:
    ret = mrb_str_new(mrb, json_value_get_string(value), json_value_get_string_len(value));
    break;
#ifdef JSON_FIXED_NUMBER
  case JSONFixed:
    ret = json_int_value(mrb, json_value_get_fixed(value));
    break;
  case JSONNumber:
#ifndef MRB_WITHOUT_FLOAT
    ret = mrb_float_value(mrb, json_value_get_number(value));
#else
    mrb_raise(mrb, E_ARGUMENT_ERROR, "float value received in non-float environment!");
#endif
    break;
#else
  case JSONNumber:
    {
      double d = json_value_get_number(value);
      if (floor(d) == d) {
        ret = mrb_fixnum_value(d);
      }
      else {
        ret = mrb_float_value(mrb, d);
      }
    }
    break;
#endif
  case JSONObject:
    {
      JSON_Object* object = json_value_get_object(value);
      size_t count = json_object_get_count(object);
      mrb_value hash = mrb_hash_new_capa(mrb, (mrb_int)count);
      size_t n;
      for (n = 0; n < count; n++) {
        int ai = mrb_gc_arena_save(mrb);
        const char* name = json_object_get_name(object, n);
        /* Look the value up by position; by name is a linear search per key. */
        mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, name),
          json_value_to_mrb_value(mrb, json_object_get_value_at(object, n)));
        mrb_gc_arena_restore(mrb, ai);
      }
      ret = hash;
    }
    break;
  case JSONArray:
    {
      mrb_value ary;
      JSON_Array* array;
      size_t n, count;
      array = json_value_get_array(value);
      count = json_array_get_count(array);
      ary = mrb_ary_new_capa(mrb, (mrb_int)count);
      for (n = 0; n < count; n++) {
        int ai = mrb_gc_arena_save(mrb);
        JSON_Value* elem = json_array_get_value(array, n);
        mrb_ary_push(mrb, ary, json_value_to_mrb_value(mrb, elem));
        mrb_gc_arena_restore(mrb, ai);
      }
      ret = ary;
    }
    break;
  case JSONBoolean:
    if (json_value_get_boolean(value))
      ret = mrb_true_value();
    else
      ret = mrb_false_value();
    break;
  default:
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }
  return ret;
}

/* Parses the whole of json; trailing bytes other than whitespace are an error. */
static mrb_value
json_parse(mrb_state *mrb, mrb_value json)
{
  mrb_value value;
  JSON_Value *root_value;
  const char *ptr, *end = NULL;

  if (memchr(RSTRING_PTR(json), '\0', RSTRING_LEN(json))) {
    mrb_raise(mrb, E_PARSER_ERROR, "invalid json");
  }
  ptr = mrb_str_to_cstr(mrb, json);
  root_value = json_parse_string_end(ptr, &end);
  if (root_value && end) {
    while (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r') end++;
  }
  if (!root_value || !end || *end != '\0') {
    if (root_value) json_value_free(root_value);
    mrb_raise(mrb, E_PARSER_ERROR, "invalid json");
  }

  value = json_value_to_mrb_value(mrb, root_value);
  json_value_free(root_value);
  return value;
}

static mrb_value
mrb_json_load(mrb_state *mrb, mrb_value self)
{
  mrb_value value, blk;
  mrb_value json = mrb_nil_value();
  mrb_get_args(mrb, "S&", &json, &blk);

  value = json_parse(mrb, json);
  if (!mrb_nil_p(blk)) {
    mrb_value args[1];
    args[0] = value;
    mrb_yield_argv(mrb, blk, 1, args);
  }
  return value;
}

static mrb_value
mrb_json_parse(mrb_state *mrb, mrb_value self)
{
  mrb_value json = mrb_nil_value();
  mrb_get_args(mrb, "S", &json);

  return json_parse(mrb, json);
}

static mrb_value
mrb_json_dump(mrb_state *mrb, mrb_value self) {
  mrb_value obj, io = mrb_nil_value(), out;
  mrb_get_args(mrb, "o|o", &obj, &io);
  out = mrb_value_to_string(mrb, obj, -1);
  if (mrb_nil_p(io)) {
    return out;
  }
  mrb_funcall(mrb, io, "write", 1, out);
  return io;
}

static mrb_value
mrb_json_generate(mrb_state *mrb, mrb_value self) {
  mrb_value obj;
  mrb_get_args(mrb, "o", &obj);
  return mrb_value_to_string(mrb, obj, -1);
}

static mrb_value
mrb_json_pretty_generate(mrb_state *mrb, mrb_value self) {
  mrb_value obj;
  mrb_get_args(mrb, "o", &obj);
  return mrb_value_to_string(mrb, obj, 0);
}

static mrb_value
mrb_json_to_json(mrb_state *mrb, mrb_value self) {
  mrb_value str;
  if (json_builtin_p(self)) {
    return mrb_value_to_string(mrb, self, -1);
  }
  str = mrb_str_new_capa(mrb, 64);
  json_cat_string(mrb, str, mrb_obj_as_string(mrb, self));
  return str;
}
/*********************************************************
 * register
 *********************************************************/

void
mrb_mruby_json_gem_init(mrb_state* mrb) {
  struct RClass *_class_json = mrb_define_module(mrb, "JSON");

  mrb_define_class_method(mrb, _class_json, "load", mrb_json_load, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, _class_json, "parse", mrb_json_parse, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, _class_json, "stringify", mrb_json_generate, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, _class_json, "dump", mrb_json_dump, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, _class_json, "generate", mrb_json_generate, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, _class_json, "pretty_generate", mrb_json_pretty_generate, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mrb->object_class, "to_json", mrb_json_to_json, MRB_ARGS_NONE());
}

void
mrb_mruby_json_gem_final(mrb_state* mrb) {
}

/* vim:set et ts=2 sts=2 sw=2 tw=0: */
