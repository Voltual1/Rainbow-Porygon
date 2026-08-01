import java.util.Properties

plugins {
    alias(libs.plugins.android.application)
}

android {
    val keystorePropertiesFile = rootProject.file("keystore.properties")
    val keystoreProperties = Properties()
    if (keystorePropertiesFile.exists()) {
        keystoreProperties.load(keystorePropertiesFile.inputStream())
    }

    namespace = "me.voltual.rp.legacy"
    compileSdk = 37
    ndkVersion = "25.2.9519653"

    base {
        archivesName.set("Rainbow-Porygon")
    }

    defaultConfig {
        applicationId = "me.voltual.rp.legacy"
        minSdk = 19
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"
        multiDexEnabled = true
        buildConfigField("String", "LICENSE", "\"GPLv3\"")

        externalNativeBuild {
            cmake {
                arguments("-DCMAKE_BUILD_TYPE=RelWithDebInfo", "-DANDROID_STL=c++_static")
            }
        }
    }

    androidResources {
        localeFilters += "zh"
    }

    signingConfigs {
        create("release") {
            storeFile = file(System.getenv("KEYSTORE_PATH") ?: keystoreProperties.getProperty("storeFile") ?: "debug.keystore")
            storePassword = System.getenv("KEYSTORE_PASSWORD") ?: keystoreProperties.getProperty("storePassword")
            keyAlias = System.getenv("KEY_ALIAS") ?: keystoreProperties.getProperty("keyAlias")
            keyPassword = System.getenv("KEY_PASSWORD") ?: keystoreProperties.getProperty("keyPassword")
        }
    }

    splits {
        abi {
            isEnable = true
            reset()
            include("armeabi-v7a",/* "arm64-v8a"*/)
            isUniversalApk = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
        isCoreLibraryDesugaringEnabled = true
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            signingConfig = signingConfigs.getByName("release")
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
        debug {
            isDebuggable = true
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("release")
        }
    }

    buildFeatures {
        buildConfig = true
    }

    packaging {
        resources {
            excludes.add("/META-INF/{AL2.0,LGPL2.1}")
            excludes.add("/META-INF/INDEX.LIST")
            excludes.add("/META-INF/DEPENDENCIES")
            excludes.add("/google/protobuf/**")
            excludes.add("/src/google/protobuf/**")
            excludes.add("/java/core/java_features_proto-descriptor-set.proto.bin")
            excludes.add("/META-INF/LICENSE*")
            excludes.add("/META-INF/*.txt")
            excludes.add("/DebugProbesKt.bin")
            merges.add("/META-INF/services/**")
        }
    }

    kotlin {
        jvmToolchain(17)
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main") {
            java.srcDirs("../SDL2/android-project/app/src/main/java")
            assets.srcDir(layout.buildDirectory.dir("generated/borderAssets"))
        }
    }
}

val copyBorderAsset = tasks.register<Sync>("copyBorderAsset") {
    from(rootProject.file("..")) {
        include("BG*.png")
        include("Border.png")
    }
    into(layout.buildDirectory.dir("generated/borderAssets"))
}

tasks.named("preBuild").configure {
    dependsOn(copyBorderAsset)
}