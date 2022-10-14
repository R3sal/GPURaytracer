
workspace "GPURaytracer"

    configurations { "Debug", "Release" }
    platforms { "x64", "x86" }


project "GPURaytracer"

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"
    targetdir ("bin/%{cfg.buildcfg}/%{cfg.platform}")
    objdir ("bin/%{cfg.buildcfg}/%{cfg.platform}/intermediate")

    files
    {
        "src/**.h",
        "src/**.cpp",
        "src/**.hlsli",
        "src/**.hlsl",
        "lib/**.lib"
    }

    includedirs
    {
        "src",
        "include"
    }

    libdirs
    {
        "lib/%{cfg.platform}"
    }

    links
    {
        "GLFW/glfw3.lib"
    }

    defines
    {
        "_CONSOLE"
    }


    -- configure different build configurations and architectures
    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:x64"
        defines { "x64" }
    filter "platforms:x86"
        defines { "x86" }

    --filter {} -- reset the filter

    -- configure shader compiler
    filter "files:**.hlsl"
        shadermodel "6.0"
        shaderobjectfileoutput("%{file.directory}/bin/%{file.basename}.cso")
    filter "files:**CS_*.hlsl"
        shadertype "Compute"
    filter "files:**VS_*.hlsl"
        shadertype "Vertex"
    filter "files:**PS_*.hlsl"
        shadertype "Pixel"
    filter "files:**CS_Intersection.hlsl"
        flags "ExcludeFromBuild"
    filter "files:*.hlsli"
        flags "ExcludeFromBuild"