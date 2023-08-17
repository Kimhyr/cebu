add_rules("mode.debug", "mode.release")

set_languages("c++20")
set_warnings("allextra")
set_toolchains("clang")
add_includedirs(".")

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
elseif is_mode("release") then
    set_symbols("hidden")
    set_optimize("fast")
    set_strip("all")
end

target("cebu", {
    kind = "binary",
    files = "cebu/**.cpp",
    pcxxheader = "cebu/precompile.h",
})
