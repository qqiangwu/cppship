#include "cppship/core/cfg.h"
#include "cppship/exception.h"

#include <string>

#include <boost/phoenix/bind/bind_member_variable.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/stl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>

using namespace cppship;
using namespace cppship::core;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

namespace {

/*
Syntax
ConfigurationPredicate :
      ConfigurationOption
   | ConfigurationAll
   | ConfigurationAny
   | ConfigurationNot

ConfigurationOption :
   [IDENTIFIER] (= ([STRING_LITERAL] | [RAW_STRING_LITERAL]))?

ConfigurationAll
   all ( ConfigurationPredicateList? )

ConfigurationAny
   any ( ConfigurationPredicateList? )

ConfigurationNot
   not ( ConfigurationPredicate )

ConfigurationPredicateList
   ConfigurationPredicate (, ConfigurationPredicate)* ,?
*/
struct CfgParser : qi::grammar<std::string_view::const_iterator, CfgPredicate(), ascii::space_type> {
    using Iterator = std::string_view::const_iterator;

    CfgParser()
        : CfgParser::base_type(cfg, "Cfg")
    {
        using phoenix::bind;
        using phoenix::push_back;
        using qi::_1;
        using qi::_a;
        using qi::_val;
        using qi::lit;

        os_symbols.name("os");
        compiler_symbols.name("compiler");

        cfg.name("Cfg");
        cfg_predicate.name("CfgPredicate");
        cfg_option.name("CfgOption");
        cfg_all.name("CfgAll");
        cfg_any.name("CfgAny");
        cfg_not.name("CfgNot");
        cfg_predicate_list.name("CfgPredicateList");

        os_symbols.add("windows", cfg::Os::windows);
        os_symbols.add("linux", cfg::Os::linux);
        os_symbols.add("macos", cfg::Os::macos);

        compiler_symbols.add("gcc", cfg::Compiler::gcc);
        compiler_symbols.add("msvc", cfg::Compiler::msvc);
        compiler_symbols.add("clang", cfg::Compiler::clang);
        compiler_symbols.add("apple_clang", cfg::Compiler::apple_clang);

        cfg_option
            = (lit("os") > '=' > '"' > os_symbols > '"') | (lit("compiler") > '=' > '"' > compiler_symbols > '"');
        cfg_predicate_list = cfg_predicate[push_back(_val, _1)] % ',';
        cfg_all = lit("all") > '(' > cfg_predicate_list[bind(&CfgAll::predicates, _val) = _1] > ')';
        cfg_any = lit("any") > '(' > cfg_predicate_list[bind(&CfgAny::predicates, _val) = _1] > ')';
        cfg_not = lit("not") > '(' > cfg_predicate[bind(&CfgNot::predicate, _val) = _1] > ')';
        cfg_predicate = cfg_option | cfg_all | cfg_any | cfg_not;
        cfg = lit("cfg") > '(' > cfg_predicate > ')';
    }

    qi::symbols<char, cfg::Os> os_symbols;
    qi::symbols<char, cfg::Compiler> compiler_symbols;

    qi::rule<Iterator, CfgPredicate(), ascii::space_type> cfg;
    qi::rule<Iterator, CfgPredicate(), ascii::space_type> cfg_predicate;
    qi::rule<Iterator, CfgOption(), ascii::space_type> cfg_option;
    qi::rule<Iterator, CfgAll(), ascii::space_type> cfg_all;
    qi::rule<Iterator, CfgAny(), ascii::space_type> cfg_any;
    qi::rule<Iterator, CfgNot(), ascii::space_type> cfg_not;
    qi::rule<Iterator, std::vector<CfgPredicate>(), ascii::space_type> cfg_predicate_list;
};

}

CfgPredicate core::parse_cfg(const std::string_view cfg)
try {
    CfgPredicate cfg_parsed;
    const bool ok = qi::phrase_parse(cfg.begin(), cfg.end(), CfgParser {}, ascii::space, cfg_parsed);
    if (!ok) {
        throw ManifestCfgParseError("cfg", cfg);
    }
    return cfg_parsed;
} catch (const qi::expectation_failure<CfgParser::iterator_type>& e) {
    throw ManifestCfgParseError(fmt::format("{}", fmt::streamed(e.what_)), std::string(e.first, e.last));
}