// Copyright 2012 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "utils/config/tree.ipp"

#include <atf-c++.hpp>

#include "utils/config/nodes.ipp"
#include "utils/format/macros.hpp"
#include "utils/text/operations.ipp"

namespace config = utils::config;
namespace text = utils::text;


namespace {


/// Simple wrapper around an integer value without default constructors.
///
/// The purpose of this type is to have a simple class without default
/// constructors to validate that we can use it as a leaf of a tree.
class int_wrapper {
    /// The wrapped integer value.
    int _value;

public:
    /// Constructs a new wrapped integer.
    ///
    /// \param value_ The value to store in the object.
    explicit int_wrapper(int value_) :
        _value(value_)
    {
    }

    /// Gets the integer value stored in the object.
    int
    value(void) const
    {
        return _value;
    }
};


/// Custom tree leaf type for an object without defualt constructors.
class wrapped_int_node : public config::typed_leaf_node< int_wrapper > {
public:
    /// Pushes the node's value onto the Lua stack.
    ///
    /// \param state The Lua state onto which to push the value.
    void
    push_lua(lutok::state& state) const
    {
        state.push_integer(
            config::typed_leaf_node< int_wrapper >::value().value());
    }

    /// Sets the value of the node from an entry in the Lua stack.
    ///
    /// \param state The Lua state from which to get the value.
    /// \param value_index The stack index in which the value resides.
    void
    set_lua(lutok::state& state, const int value_index)
    {
        ATF_REQUIRE(state.is_number(value_index));
        int_wrapper new_value(state.to_integer(value_index));
        config::typed_leaf_node< int_wrapper >::set(new_value);
    }

    /// Sets the value of the node from a raw string representation.
    ///
    /// \param raw_value The value to set the node to.
    void
    set_string(const std::string& raw_value)
    {
        int_wrapper new_value(text::to_type< int >(raw_value));
        config::typed_leaf_node< int_wrapper >::set(new_value);
    }

    /// Converts the contents of the node to a string.
    ///
    /// \return A string representation of the value held by the node.
    std::string
    to_string(void) const
    {
        return F("%s") %
            config::typed_leaf_node< int_wrapper >::value().value();
    }
};


}  // anonymous namespace


ATF_TEST_CASE_WITHOUT_HEAD(define_set_lookup__one_level);
ATF_TEST_CASE_BODY(define_set_lookup__one_level)
{
    config::tree tree;

    tree.define< config::int_node >("var1");
    tree.define< config::string_node >("var2");
    tree.define< config::bool_node >("var3");

    tree.set< config::int_node >("var1", 42);
    tree.set< config::string_node >("var2", "hello");
    tree.set< config::bool_node >("var3", false);

    ATF_REQUIRE_EQ(42, tree.lookup< config::int_node >("var1"));
    ATF_REQUIRE_EQ("hello", tree.lookup< config::string_node >("var2"));
    ATF_REQUIRE(!tree.lookup< config::bool_node >("var3"));
}


ATF_TEST_CASE_WITHOUT_HEAD(define_set_lookup__multiple_levels);
ATF_TEST_CASE_BODY(define_set_lookup__multiple_levels)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar.1");
    tree.define< config::string_node >("foo.bar.2");
    tree.define< config::bool_node >("foo.3");
    tree.define_dynamic("sub.tree");

    tree.set< config::int_node >("foo.bar.1", 42);
    tree.set< config::string_node >("foo.bar.2", "hello");
    tree.set< config::bool_node >("foo.3", true);
    tree.set< config::string_node >("sub.tree.1", "bye");
    tree.set< config::int_node >("sub.tree.2", 4);
    tree.set< config::int_node >("sub.tree.3.4", 123);

    ATF_REQUIRE_EQ(42, tree.lookup< config::int_node >("foo.bar.1"));
    ATF_REQUIRE_EQ("hello", tree.lookup< config::string_node >("foo.bar.2"));
    ATF_REQUIRE(tree.lookup< config::bool_node >("foo.3"));
    ATF_REQUIRE_EQ(4, tree.lookup< config::int_node >("sub.tree.2"));
    ATF_REQUIRE_EQ(123, tree.lookup< config::int_node >("sub.tree.3.4"));
}


ATF_TEST_CASE_WITHOUT_HEAD(lookup__invalid_key);
ATF_TEST_CASE_BODY(lookup__invalid_key)
{
    config::tree tree;

    ATF_REQUIRE_THROW(config::invalid_key_error,
                      tree.lookup< config::int_node >("."));
}


ATF_TEST_CASE_WITHOUT_HEAD(lookup__unknown_key);
ATF_TEST_CASE_BODY(lookup__unknown_key)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar");
    tree.define< config::int_node >("a.b.c");
    tree.define_dynamic("a.d");
    tree.set< config::int_node >("a.b.c", 123);
    tree.set< config::int_node >("a.d.100", 0);

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("abc"));

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("foo"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("foo.bar"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("foo.bar.baz"));

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.b"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.c"));
    (void)tree.lookup< config::int_node >("a.b.c");
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.b.c.d"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.d"));
    (void)tree.lookup< config::int_node >("a.d.100");
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.d.101"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.d.100.3"));
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.lookup< config::int_node >("a.d.e"));
}


ATF_TEST_CASE_WITHOUT_HEAD(is_set__one_level);
ATF_TEST_CASE_BODY(is_set__one_level)
{
    config::tree tree;

    tree.define< config::int_node >("var1");
    tree.define< config::string_node >("var2");
    tree.define< config::bool_node >("var3");

    tree.set< config::int_node >("var1", 42);
    tree.set< config::bool_node >("var3", false);

    ATF_REQUIRE( tree.is_set("var1"));
    ATF_REQUIRE(!tree.is_set("var2"));
    ATF_REQUIRE( tree.is_set("var3"));
}


ATF_TEST_CASE_WITHOUT_HEAD(is_set__multiple_levels);
ATF_TEST_CASE_BODY(is_set__multiple_levels)
{
    config::tree tree;

    tree.define< config::int_node >("a.b.var1");
    tree.define< config::string_node >("a.b.var2");
    tree.define< config::bool_node >("e.var3");

    tree.set< config::int_node >("a.b.var1", 42);
    tree.set< config::bool_node >("e.var3", false);

    ATF_REQUIRE(!tree.is_set("a"));
    ATF_REQUIRE(!tree.is_set("a.b"));
    ATF_REQUIRE( tree.is_set("a.b.var1"));
    ATF_REQUIRE(!tree.is_set("a.b.var1.trailing"));

    ATF_REQUIRE(!tree.is_set("a"));
    ATF_REQUIRE(!tree.is_set("a.b"));
    ATF_REQUIRE(!tree.is_set("a.b.var2"));
    ATF_REQUIRE(!tree.is_set("a.b.var2.trailing"));

    ATF_REQUIRE(!tree.is_set("e"));
    ATF_REQUIRE( tree.is_set("e.var3"));
    ATF_REQUIRE(!tree.is_set("e.var3.trailing"));
}


ATF_TEST_CASE_WITHOUT_HEAD(is_set__invalid_key);
ATF_TEST_CASE_BODY(is_set__invalid_key)
{
    config::tree tree;

    ATF_REQUIRE_THROW(config::invalid_key_error, tree.is_set(".abc"));
}


ATF_TEST_CASE_WITHOUT_HEAD(set__invalid_key);
ATF_TEST_CASE_BODY(set__invalid_key)
{
    config::tree tree;

    ATF_REQUIRE_THROW(config::invalid_key_error,
                      tree.set< config::int_node >("foo.", 54));
}


ATF_TEST_CASE_WITHOUT_HEAD(set__unknown_key);
ATF_TEST_CASE_BODY(set__unknown_key)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar");
    tree.define< config::int_node >("a.b.c");
    tree.define_dynamic("a.d");
    tree.set< config::int_node >("a.b.c", 123);
    tree.set< config::string_node >("a.d.3", "foo");

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set< config::int_node >("abc", 2));

    tree.set< config::int_node >("foo.bar", 15);
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set< config::int_node >("foo.bar.baz", 0));

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set< config::int_node >("a.c", 100));
    tree.set< config::int_node >("a.b.c", -3);
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set< config::int_node >("a.b.c.d", 82));
    tree.set< config::string_node >("a.d.3", "bar");
    tree.set< config::string_node >("a.d.4", "bar");
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set< config::int_node >("a.d.4.5", 82));
    tree.set< config::int_node >("a.d.5.6", 82);
}


ATF_TEST_CASE_WITHOUT_HEAD(set__value_error);
ATF_TEST_CASE_BODY(set__value_error)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar");
    tree.define_dynamic("a.d");

    ATF_REQUIRE_THROW(config::value_error,
                      tree.set< config::int_node >("foo", 3));
    ATF_REQUIRE_THROW(config::value_error,
                      tree.set< config::int_node >("a", -10));
}


ATF_TEST_CASE_WITHOUT_HEAD(push_lua__ok);
ATF_TEST_CASE_BODY(push_lua__ok)
{
    config::tree tree;

    tree.define< config::int_node >("top.integer");
    tree.define< wrapped_int_node >("top.custom");
    tree.define_dynamic("dynamic");
    tree.set< config::int_node >("top.integer", 5);
    tree.set< wrapped_int_node >("top.custom", int_wrapper(10));
    tree.set_string("dynamic.first", "foo");

    lutok::state state;
    tree.push_lua("top.integer", state);
    tree.push_lua("top.custom", state);
    tree.push_lua("dynamic.first", state);
    ATF_REQUIRE(state.is_number(-3));
    ATF_REQUIRE_EQ(5, state.to_integer(-3));
    ATF_REQUIRE(state.is_number(-2));
    ATF_REQUIRE_EQ(10, state.to_integer(-2));
    ATF_REQUIRE(state.is_string(-1));
    ATF_REQUIRE_EQ("foo", state.to_string(-1));
    state.pop(3);
}


ATF_TEST_CASE_WITHOUT_HEAD(set_lua__ok);
ATF_TEST_CASE_BODY(set_lua__ok)
{
    config::tree tree;

    tree.define< config::int_node >("top.integer");
    tree.define< wrapped_int_node >("top.custom");
    tree.define_dynamic("dynamic");

    {
        lutok::state state;
        state.push_integer(5);
        state.push_integer(10);
        state.push_string("foo");
        tree.set_lua("top.integer", state, -3);
        tree.set_lua("top.custom", state, -2);
        tree.set_lua("dynamic.first", state, -1);
        state.pop(3);
    }

    ATF_REQUIRE_EQ(5, tree.lookup< config::int_node >("top.integer"));
    ATF_REQUIRE_EQ(10, tree.lookup< wrapped_int_node >("top.custom").value());
    ATF_REQUIRE_EQ("foo", tree.lookup< config::string_node >("dynamic.first"));
}


ATF_TEST_CASE_WITHOUT_HEAD(lookup_string__ok);
ATF_TEST_CASE_BODY(lookup_string__ok)
{
    config::tree tree;

    tree.define< config::int_node >("var1");
    tree.define< config::string_node >("b.var2");
    tree.define< config::bool_node >("c.d.var3");

    tree.set< config::int_node >("var1", 42);
    tree.set< config::string_node >("b.var2", "hello");
    tree.set< config::bool_node >("c.d.var3", false);

    ATF_REQUIRE_EQ("42", tree.lookup_string("var1"));
    ATF_REQUIRE_EQ("hello", tree.lookup_string("b.var2"));
    ATF_REQUIRE_EQ("false", tree.lookup_string("c.d.var3"));
}


ATF_TEST_CASE_WITHOUT_HEAD(lookup_string__invalid_key);
ATF_TEST_CASE_BODY(lookup_string__invalid_key)
{
    config::tree tree;

    ATF_REQUIRE_THROW(config::invalid_key_error, tree.lookup_string(""));
}


ATF_TEST_CASE_WITHOUT_HEAD(lookup_string__unknown_key);
ATF_TEST_CASE_BODY(lookup_string__unknown_key)
{
    config::tree tree;

    tree.define< config::int_node >("a.b.c");

    ATF_REQUIRE_THROW(config::unknown_key_error, tree.lookup_string("a.b"));
    ATF_REQUIRE_THROW(config::unknown_key_error, tree.lookup_string("a.b.c.d"));
}


ATF_TEST_CASE_WITHOUT_HEAD(set_string__ok);
ATF_TEST_CASE_BODY(set_string__ok)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar.1");
    tree.define< config::string_node >("foo.bar.2");
    tree.define_dynamic("sub.tree");

    tree.set_string("foo.bar.1", "42");
    tree.set_string("foo.bar.2", "hello");
    tree.set_string("sub.tree.2", "15");
    tree.set_string("sub.tree.3.4", "bye");

    ATF_REQUIRE_EQ(42, tree.lookup< config::int_node >("foo.bar.1"));
    ATF_REQUIRE_EQ("hello", tree.lookup< config::string_node >("foo.bar.2"));
    ATF_REQUIRE_EQ("15", tree.lookup< config::string_node >("sub.tree.2"));
    ATF_REQUIRE_EQ("bye", tree.lookup< config::string_node >("sub.tree.3.4"));
}


ATF_TEST_CASE_WITHOUT_HEAD(set_string__invalid_key);
ATF_TEST_CASE_BODY(set_string__invalid_key)
{
    config::tree tree;

    ATF_REQUIRE_THROW(config::invalid_key_error, tree.set_string(".", "foo"));
}


ATF_TEST_CASE_WITHOUT_HEAD(set_string__unknown_key);
ATF_TEST_CASE_BODY(set_string__unknown_key)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar");
    tree.define< config::int_node >("a.b.c");
    tree.define_dynamic("a.d");
    tree.set_string("a.b.c", "123");
    tree.set_string("a.d.3", "foo");

    ATF_REQUIRE_THROW(config::unknown_key_error, tree.set_string("abc", "2"));

    tree.set_string("foo.bar", "15");
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set_string("foo.bar.baz", "0"));

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set_string("a.c", "100"));
    tree.set_string("a.b.c", "-3");
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set_string("a.b.c.d", "82"));
    tree.set_string("a.d.3", "bar");
    tree.set_string("a.d.4", "bar");
    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.set_string("a.d.4.5", "82"));
    tree.set_string("a.d.5.6", "82");
}


ATF_TEST_CASE_WITHOUT_HEAD(set_string__value_error);
ATF_TEST_CASE_BODY(set_string__value_error)
{
    config::tree tree;

    tree.define< config::int_node >("foo.bar");

    ATF_REQUIRE_THROW(config::value_error,
                      tree.set_string("foo", "abc"));
    ATF_REQUIRE_THROW(config::value_error,
                      tree.set_string("foo.bar", " -3"));
    ATF_REQUIRE_THROW(config::value_error,
                      tree.set_string("foo.bar", "3 "));
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__none);
ATF_TEST_CASE_BODY(all_properties__none)
{
    const config::tree tree;
    ATF_REQUIRE(tree.all_properties().empty());
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__all_set);
ATF_TEST_CASE_BODY(all_properties__all_set)
{
    config::tree tree;

    tree.define< config::int_node >("plain");
    tree.set< config::int_node >("plain", 1234);

    tree.define< config::int_node >("static.first");
    tree.set< config::int_node >("static.first", -3);
    tree.define< config::string_node >("static.second");
    tree.set< config::string_node >("static.second", "some text");

    tree.define_dynamic("dynamic");
    tree.set< config::string_node >("dynamic.first", "hello");
    tree.set< config::string_node >("dynamic.second", "bye");

    config::properties_map exp_properties;
    exp_properties["plain"] = "1234";
    exp_properties["static.first"] = "-3";
    exp_properties["static.second"] = "some text";
    exp_properties["dynamic.first"] = "hello";
    exp_properties["dynamic.second"] = "bye";

    const config::properties_map properties = tree.all_properties();
    ATF_REQUIRE(exp_properties == properties);
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__some_unset);
ATF_TEST_CASE_BODY(all_properties__some_unset)
{
    config::tree tree;

    tree.define< config::int_node >("static.first");
    tree.set< config::int_node >("static.first", -3);
    tree.define< config::string_node >("static.second");

    tree.define_dynamic("dynamic");

    config::properties_map exp_properties;
    exp_properties["static.first"] = "-3";

    const config::properties_map properties = tree.all_properties();
    ATF_REQUIRE(exp_properties == properties);
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__subtree__inner);
ATF_TEST_CASE_BODY(all_properties__subtree__inner)
{
    config::tree tree;

    tree.define< config::int_node >("root.a.b.c.first");
    tree.define< config::int_node >("root.a.b.c.second");
    tree.define< config::int_node >("root.a.d.first");

    tree.set< config::int_node >("root.a.b.c.first", 1);
    tree.set< config::int_node >("root.a.b.c.second", 2);
    tree.set< config::int_node >("root.a.d.first", 3);

    {
        config::properties_map exp_properties;
        exp_properties["root.a.b.c.first"] = "1";
        exp_properties["root.a.b.c.second"] = "2";
        exp_properties["root.a.d.first"] = "3";
        ATF_REQUIRE(exp_properties == tree.all_properties("root"));
        ATF_REQUIRE(exp_properties == tree.all_properties("root.a"));
    }

    {
        config::properties_map exp_properties;
        exp_properties["root.a.b.c.first"] = "1";
        exp_properties["root.a.b.c.second"] = "2";
        ATF_REQUIRE(exp_properties == tree.all_properties("root.a.b"));
        ATF_REQUIRE(exp_properties == tree.all_properties("root.a.b.c"));
    }

    {
        config::properties_map exp_properties;
        exp_properties["root.a.d.first"] = "3";
        ATF_REQUIRE(exp_properties == tree.all_properties("root.a.d"));
    }
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__subtree__leaf);
ATF_TEST_CASE_BODY(all_properties__subtree__leaf)
{
    config::tree tree;

    tree.define< config::int_node >("root.a.b.c.first");
    tree.set< config::int_node >("root.a.b.c.first", 1);
    ATF_REQUIRE_THROW_RE(config::value_error, "Cannot export.*leaf",
                         tree.all_properties("root.a.b.c.first"));
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__subtree__strip_key);
ATF_TEST_CASE_BODY(all_properties__subtree__strip_key)
{
    config::tree tree;

    tree.define< config::int_node >("root.a.b.c.first");
    tree.define< config::int_node >("root.a.b.c.second");
    tree.define< config::int_node >("root.a.d.first");

    tree.set< config::int_node >("root.a.b.c.first", 1);
    tree.set< config::int_node >("root.a.b.c.second", 2);
    tree.set< config::int_node >("root.a.d.first", 3);

    config::properties_map exp_properties;
    exp_properties["b.c.first"] = "1";
    exp_properties["b.c.second"] = "2";
    exp_properties["d.first"] = "3";
    ATF_REQUIRE(exp_properties == tree.all_properties("root.a", true));
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__subtree__invalid_key);
ATF_TEST_CASE_BODY(all_properties__subtree__invalid_key)
{
    config::tree tree;

    ATF_REQUIRE_THROW(config::invalid_key_error, tree.all_properties("."));
}


ATF_TEST_CASE_WITHOUT_HEAD(all_properties__subtree__unknown_key);
ATF_TEST_CASE_BODY(all_properties__subtree__unknown_key)
{
    config::tree tree;

    tree.define< config::int_node >("root.a.b.c.first");
    tree.set< config::int_node >("root.a.b.c.first", 1);
    tree.define< config::int_node >("root.a.b.c.unset");

    ATF_REQUIRE_THROW(config::unknown_key_error,
                      tree.all_properties("root.a.b.c.first.foo"));
    ATF_REQUIRE_THROW_RE(config::value_error, "Cannot export.*leaf",
                         tree.all_properties("root.a.b.c.unset"));
}


ATF_TEST_CASE_WITHOUT_HEAD(custom_leaf__no_default_ctor);
ATF_TEST_CASE_BODY(custom_leaf__no_default_ctor)
{
    config::tree tree;

    tree.define< wrapped_int_node >("test1");
    tree.define< wrapped_int_node >("test2");
    tree.set< wrapped_int_node >("test1", int_wrapper(5));
    tree.set< wrapped_int_node >("test2", int_wrapper(10));
    const int_wrapper& test1 = tree.lookup< wrapped_int_node >("test1");
    ATF_REQUIRE_EQ(5, test1.value());
    const int_wrapper& test2 = tree.lookup< wrapped_int_node >("test2");
    ATF_REQUIRE_EQ(10, test2.value());
}


ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, define_set_lookup__one_level);
    ATF_ADD_TEST_CASE(tcs, define_set_lookup__multiple_levels);

    ATF_ADD_TEST_CASE(tcs, lookup__invalid_key);
    ATF_ADD_TEST_CASE(tcs, lookup__unknown_key);

    ATF_ADD_TEST_CASE(tcs, is_set__one_level);
    ATF_ADD_TEST_CASE(tcs, is_set__multiple_levels);
    ATF_ADD_TEST_CASE(tcs, is_set__invalid_key);

    ATF_ADD_TEST_CASE(tcs, set__invalid_key);
    ATF_ADD_TEST_CASE(tcs, set__unknown_key);
    ATF_ADD_TEST_CASE(tcs, set__value_error);

    ATF_ADD_TEST_CASE(tcs, push_lua__ok);
    ATF_ADD_TEST_CASE(tcs, set_lua__ok);

    ATF_ADD_TEST_CASE(tcs, lookup_string__ok);
    ATF_ADD_TEST_CASE(tcs, lookup_string__invalid_key);
    ATF_ADD_TEST_CASE(tcs, lookup_string__unknown_key);

    ATF_ADD_TEST_CASE(tcs, set_string__ok);
    ATF_ADD_TEST_CASE(tcs, set_string__invalid_key);
    ATF_ADD_TEST_CASE(tcs, set_string__unknown_key);
    ATF_ADD_TEST_CASE(tcs, set_string__value_error);

    ATF_ADD_TEST_CASE(tcs, all_properties__none);
    ATF_ADD_TEST_CASE(tcs, all_properties__all_set);
    ATF_ADD_TEST_CASE(tcs, all_properties__some_unset);
    ATF_ADD_TEST_CASE(tcs, all_properties__subtree__inner);
    ATF_ADD_TEST_CASE(tcs, all_properties__subtree__leaf);
    ATF_ADD_TEST_CASE(tcs, all_properties__subtree__strip_key);
    ATF_ADD_TEST_CASE(tcs, all_properties__subtree__invalid_key);
    ATF_ADD_TEST_CASE(tcs, all_properties__subtree__unknown_key);

    ATF_ADD_TEST_CASE(tcs, custom_leaf__no_default_ctor);
}