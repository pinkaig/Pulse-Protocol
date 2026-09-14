# Build ManagedScripts C# assembly using dotnet CLI
# The .csproj OutputPath already points to build/.bin/{Config}-x64/
# so we don't pass -o here — dotnet respects the .csproj settings.
add_custom_target(ManagedScripts ALL
    COMMAND dotnet build "${CMAKE_CURRENT_SOURCE_DIR}/ManagedScripts.csproj"
            -c $<CONFIG>
            -p:Platform=x64
    BYPRODUCTS "${CMAKE_BINARY_DIR}/.bin/$<CONFIG>-x64/ManagedScripts.dll"
    COMMENT "Building ManagedScripts C# assembly"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ManagedScripts references ScriptAPI.dll. Must build after it!!!!
add_dependencies(ManagedScripts ScriptAPI)
