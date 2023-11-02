
using System.IO;
using UnrealBuildTool;

public class Cflat : ModuleRules
{
    public Cflat(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "Engine", "DirectoryWatcher" });

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        if (Target.Type == TargetType.Editor)
        {
            PublicDefinitions.Add("CFLAT_ENABLED");

            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../.."));
        }
        else
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../../.."));
        }
    }
}
