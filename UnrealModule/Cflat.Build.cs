
using System.IO;
using UnrealBuildTool;

public class Cflat : ModuleRules
{
    public Cflat(ReadOnlyTargetRules Target) : base(Target)
    {
        // The criteria to define "CFLAT_ENABLED" can be changed as needed
        if(Target.Type == TargetType.Editor)
        {
            PublicDefinitions.Add("CFLAT_ENABLED");
        }

        if(PublicDefinitions.Contains("CFLAT_ENABLED"))
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DirectoryWatcher", "Slate", "SlateCore" });

            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../.."));
        }
        else
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../../.."));
        }
    }
}
