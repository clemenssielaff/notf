#include "catch.hpp"

#include "app/path.hpp"

NOTF_USING_NAMESPACE

// ================================================================================================================== //

SCENARIO("Application path", "[app], [path]")
{
    SECTION("default constructed paths are empty but valid and relative")
    {
        const Path path;
        REQUIRE(path.is_empty());
        REQUIRE(path.is_relative());
    }

    SECTION("non-empty paths can be invalid")
    {
        SECTION("no nodes following a property")
        {
            REQUIRE_THROWS_AS(Path("test/node:property/wrong"), Path::construction_error);
        }
        SECTION("no properties following a property")
        {
            REQUIRE_THROWS_AS(Path("test/node:property:wrong"), Path::construction_error);
        }
        SECTION("no empty property name")
        {
            REQUIRE_THROWS_AS(Path("test/wrong:"), Path::construction_error); //
        }
        SECTION("no going up further than the root in absolute paths")
        {
            REQUIRE_THROWS_AS(Path("/root/child/../../../nope"), Path::construction_error);
        }
    }

    SECTION("valid paths are correctly identified")
    {
        SECTION("paths can be absolute or relvative")
        {
            const Path absolute("/test/this:path");
            REQUIRE_FALSE(absolute.is_empty());
            REQUIRE(absolute.is_absolute());

            const Path relative("test/this/path");
            REQUIRE_FALSE(relative.is_empty());
            REQUIRE(relative.is_relative());
        }

        SECTION("paths can denote a node or a property")
        {
            const Path node("/this/is/a/valid/path");
            REQUIRE_FALSE(node.is_empty());
            REQUIRE(node.is_node());
            REQUIRE_FALSE(node.is_property());

            const Path property("this/is/a/valid:path");
            REQUIRE_FALSE(property.is_empty());
            REQUIRE_FALSE(property.is_node());
            REQUIRE(property.is_property());
        }

        SECTION("a special relative path consists only of a single dot")
        {
            const Path current(".");
            REQUIRE(current.is_node());
            REQUIRE(current.is_relative());
        }

        SECTION("relative paths can start with 0-n '..'-tokens")
        {
            const Path empty("three/steps/down/../../..");
            REQUIRE(empty.is_empty());

            const Path one_up("two/down/../../../sibling/..");
            REQUIRE_FALSE(one_up.is_empty());
            REQUIRE(one_up.is_relative());
            REQUIRE(one_up == Path(".."));

            const Path two_up("three/steps/down/../../../../../");
            REQUIRE_FALSE(two_up.is_empty());
            REQUIRE(two_up.is_relative());
            REQUIRE(two_up == Path("../.."));
        }

        SECTION("a property can also be used as a relative path")
        {
            const Path just_property(":property");
            REQUIRE_FALSE(just_property.is_empty());
            REQUIRE(just_property.is_property());
            REQUIRE(just_property.is_relative());

            const Path node("./some/node");
            REQUIRE(node + just_property == Path("some/node:property"));
        }
    }

    SECTION("paths can be iterated")
    {
        const Path path("/parent/child/target:property");
        REQUIRE(path.size() == 4);
        REQUIRE(path[0] == "parent");
        REQUIRE(path[1] == "child");
        REQUIRE(path[2] == "target");
        REQUIRE(path[3] == "property");
        REQUIRE_THROWS_AS(path[4], Path::path_error);
    }

    SECTION("paths can be compared")
    {
        REQUIRE(Path("") == Path());
        REQUIRE(Path("") != Path(".")); // left one is ambiguous, right one is node
        REQUIRE(Path("") != Path("/")); // left one is relative, right one is absolute
        REQUIRE(Path("node") != Path("/node"));
        REQUIRE(Path("node") != Path(".node"));
        REQUIRE(Path("property") != Path(":property"));
        REQUIRE(Path("one") != Path("one/two"));
        REQUIRE(Path("one/two") != Path("one/two/three"));
        REQUIRE(Path("one/two") != Path("one/three"));
        REQUIRE(Path("one/two") == Path("one/two"));
        REQUIRE(Path("one/two:three") == Path("one/two:three"));
    }

    SECTION("relative paths can be resolved to equivalent absolute")
    {
        SECTION("by going down in the hierarchy")
        {
            const Path absolute("/parent/path/to/child");
            const Path relative("path/to/child");
            const Path resolved = Path("/parent") + relative;
            REQUIRE(resolved == absolute);
        }

        SECTION("by going up in the hierarchy")
        {
            const Path absolute("/parent/path/to/sibling");
            const Path relative("../path/to/sibling");
            const Path resolved = Path("/parent/child") + relative;
            REQUIRE(resolved == absolute);
        }

        SECTION("by explicitly referring to the current")
        {
            const Path absolute("/parent/path/to/child");
            const Path relative("./path/to/child");
            const Path resolved = Path("/parent") + relative;
            REQUIRE(resolved == absolute);
        }

        SECTION("by concatenating multiple relative paths")
        {
            const Path absolute("/parent/path/to/another/child");
            const Path relative1("path/to/");
            const Path relative2("./another");
            const Path relative3("child");
            const Path resolved = Path("/parent") + relative1 + relative2 + relative3;
            REQUIRE(resolved == absolute);
        }

        SECTION("down to child properties")
        {
            const Path absolute("/parent/path/to:property");
            const Path relative("path/to:property");
            const Path resolved = Path("/parent") + relative;
            REQUIRE(resolved == absolute);
        }

        SECTION("up from properties")
        {
            const Path absolute("/parent/path/to/another:property");
            const Path relative("../../another:property");
            const Path resolved = Path("/parent/path/to/some:property") + relative;
            REQUIRE(resolved == absolute);
        }

        SECTION("with an empty absolute graph")
        {
            const Path absolute("/path/to/child");
            const Path relative("path/to/child");
            const Path resolved = Path("/") + relative;
            REQUIRE(resolved == absolute);
        }
    }

    SECTION("absolute paths cannot be concatenated")
    {
        SECTION("to a relative path")
        {
            const Path absolute("/parent/to/absolute");
            const Path relative("path/to:property");
            REQUIRE_THROWS_AS(relative + absolute, Path::construction_error);
        }

        SECTION("to another absolute path")
        {
            const Path absolute1("/parent/to/absolute");
            const Path absolute2("/another/absolute:property");
            REQUIRE_THROWS_AS(absolute1 + absolute2, Path::construction_error);
        }
    }

    SECTION("when appending a relative path to a property path the relative path must start with '..'")
    {
        const Path start("/parent/to/absolute:property");
        const Path okay("../child/to/another:property");
        const Path should_work = start + okay;
        REQUIRE(should_work == Path("/parent/to/absolute/child/to/another:property"));
        REQUIRE(should_work.is_absolute());
        REQUIRE(should_work.is_property());

        const Path broken("nope/does/not:work");
        REQUIRE_THROWS_AS(start + broken, Path::construction_error);
    }

    SECTION("superfluous symbols are ignored")
    {
        const Path superfluous("/parent/./child/../child/target/");
        const Path normalized("/parent/child/target");
        REQUIRE(superfluous == normalized);
        REQUIRE(superfluous.size() == 3);
    }

    SECTION("rvalue paths can be combined effectively")
    {
        const Path combined = Path("/absolute") + std::string("parent") + Path(".") + "child" + Path(":property");
        REQUIRE(combined == Path("/absolute/parent/child:property"));
    }

    SECTION("single component paths can be ambiguous")
    {
        const Path ambiguous("Ambiguous");
        REQUIRE(ambiguous.is_property());
        REQUIRE(ambiguous.is_node());

        const Path property(":property");
        REQUIRE(property.is_property());
        REQUIRE(!property.is_node());

        const Path property_relative(".:property");
        REQUIRE(property_relative.is_property());
        REQUIRE(!property_relative.is_node());

        const Path property_multicomponent("node:property");
        REQUIRE(property_multicomponent.is_property());
        REQUIRE(!property_multicomponent.is_node());

        const Path node_relativ(".node");
        REQUIRE(!node_relativ.is_property());
        REQUIRE(node_relativ.is_node());

        const Path node_absolute("/node");
        REQUIRE(!node_absolute.is_property());
        REQUIRE(node_absolute.is_node());

        const Path node_multicomponent("node/other_node");
        REQUIRE(!node_multicomponent.is_property());
        REQUIRE(node_multicomponent.is_node());
    }

    SECTION("two ambiguous paths are assumed to refer to a node when concatenated")
    {
        const Path ambiguous_one("one");
        REQUIRE(ambiguous_one.is_property());
        REQUIRE(ambiguous_one.is_node());

        const Path ambiguous_two("two");
        REQUIRE(ambiguous_two.is_property());
        REQUIRE(ambiguous_two.is_node());

        const Path conatenated = ("one/two");
        REQUIRE(!conatenated.is_property());
        REQUIRE(conatenated.is_node());

        REQUIRE(ambiguous_one + ambiguous_two == conatenated);
    }

    SECTION("paths can be converted back into a string")
    {
        REQUIRE(Path().to_string() == "");
        REQUIRE(Path("ambiguous").to_string() == "ambiguous");
        REQUIRE(Path("/absolute/node").to_string() == "/absolute/node");
        REQUIRE(Path("/absolute/node:property").to_string() == "/absolute/node:property");
        REQUIRE(Path("relative/node:property").to_string() == "relative/node:property");
    }

    SECTION("paths check whether they begin with another path")
    {
        const Path absolute_node("/parent/node/absolute");
        REQUIRE(absolute_node.begins_with(Path()));
        REQUIRE(absolute_node.begins_with(Path("/")));
        REQUIRE(absolute_node.begins_with(Path("/parent")));
        REQUIRE(absolute_node.begins_with(Path("/parent/")));
        REQUIRE(absolute_node.begins_with(Path("/parent/node/")));
        REQUIRE(absolute_node.begins_with(Path("/parent/node/absolute")));
        REQUIRE(absolute_node.begins_with(Path("parent"))); // ambiguous paths are also allowed
        REQUIRE(!absolute_node.begins_with(Path(".parent")));
        REQUIRE(!absolute_node.begins_with(Path("parent/")));
        REQUIRE(!absolute_node.begins_with(Path("/blub")));
        REQUIRE(!absolute_node.begins_with(Path("/parent/blub")));
        REQUIRE(!absolute_node.begins_with(Path("/parent/node/absolute/not")));
        REQUIRE(!absolute_node.begins_with(Path("/parent/node:absolute")));

        const Path relative_node("./parent/node/relative");
        REQUIRE(relative_node.begins_with(Path()));
        REQUIRE(relative_node.begins_with(Path("./parent")));
        REQUIRE(relative_node.begins_with(Path("./parent/")));
        REQUIRE(relative_node.begins_with(Path("./parent/node")));
        REQUIRE(relative_node.begins_with(Path("./parent/node/relative")));
        REQUIRE(relative_node.begins_with(Path("parent"))); // ambiguous paths are also allowed
        REQUIRE(relative_node.begins_with(Path("parent/")));
        REQUIRE(relative_node.begins_with(Path("parent/node")));
        REQUIRE(relative_node.begins_with(Path("parent/node/relative")));
        REQUIRE(!relative_node.begins_with(Path("/")));
        REQUIRE(!relative_node.begins_with(Path("/parent")));
        REQUIRE(!relative_node.begins_with(Path("blub")));
        REQUIRE(!relative_node.begins_with(Path("parent/blub")));
        REQUIRE(!relative_node.begins_with(Path("parent/node/relative/not")));
        REQUIRE(!relative_node.begins_with(Path("parent/node/relative:not")));

        const Path ambiguous("ambiguous");
        REQUIRE(ambiguous.begins_with(Path()));
        REQUIRE(!ambiguous.begins_with(Path("/")));
        REQUIRE(!ambiguous.begins_with(Path(".")));
        REQUIRE(!ambiguous.begins_with(Path("blub")));
        REQUIRE(!ambiguous.begins_with(Path("ambiguous/not")));
        REQUIRE(!ambiguous.begins_with(Path("ambiguous:not")));

        const Path absolute_property("/parent/node:absolute");
        REQUIRE(absolute_property.begins_with(Path()));
        REQUIRE(absolute_property.begins_with(Path("/")));
        REQUIRE(absolute_property.begins_with(Path("/parent")));
        REQUIRE(absolute_property.begins_with(Path("/parent/")));
        REQUIRE(absolute_property.begins_with(Path("/parent/node/")));
        REQUIRE(absolute_property.begins_with(Path("/parent/node:absolute")));
        REQUIRE(absolute_property.begins_with(Path("parent"))); // ambiguous paths are also allowed
        REQUIRE(!absolute_property.begins_with(Path(".parent")));
        REQUIRE(!absolute_property.begins_with(Path("parent/")));
        REQUIRE(!absolute_property.begins_with(Path("/blub")));
        REQUIRE(!absolute_property.begins_with(Path("/parent/blub")));
        REQUIRE(!absolute_property.begins_with(Path("/parent/node/absolute")));
        REQUIRE(!absolute_property.begins_with(Path("/parent/node:not")));

        const Path relative_property("./parent/node:relative");
        REQUIRE(relative_property.begins_with(Path()));
        REQUIRE(relative_property.begins_with(Path("./parent")));
        REQUIRE(relative_property.begins_with(Path("./parent/")));
        REQUIRE(relative_property.begins_with(Path("./parent/node")));
        REQUIRE(relative_property.begins_with(Path("./parent/node:relative")));
        REQUIRE(relative_property.begins_with(Path("parent"))); // ambiguous paths are also allowed
        REQUIRE(relative_property.begins_with(Path("parent/")));
        REQUIRE(relative_property.begins_with(Path("parent/node")));
        REQUIRE(relative_property.begins_with(Path("parent/node:relative")));
        REQUIRE(!relative_property.begins_with(Path("/")));
        REQUIRE(!relative_property.begins_with(Path("/parent")));
        REQUIRE(!relative_property.begins_with(Path("blub")));
        REQUIRE(!relative_property.begins_with(Path("parent/blub")));
        REQUIRE(!relative_property.begins_with(Path("parent/node/relative")));
        REQUIRE(!relative_property.begins_with(Path("parent/node:not")));
    }
}
