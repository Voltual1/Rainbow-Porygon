// settings.gradle.kts
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.PREFER_PROJECT)
    
    repositories {
        google()
        mavenCentral()
        maven { url = uri("https://jitpack.io") }       
    }
}

rootProject.name = "PokeEmeraldExperimental"
include(":android")