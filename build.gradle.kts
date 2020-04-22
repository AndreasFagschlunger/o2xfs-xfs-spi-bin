plugins {
    `cpp-library`
    `maven-publish`
    `visual-studio`
}

import org.gradle.language.cpp.internal.DefaultCppBinary
import org.gradle.nativeplatform.Linkage;

group = "at.o2xfs"
version = "1.0-SNAPSHOT"

repositories {
    maven {
        url = uri("https://repo.fagschlunger.co.at/libs-snapshot-local")
    }
    maven {
        url = uri("https://repo.fagschlunger.co.at/libs-release-local")
    }
}

library {
    linkage.set(listOf(Linkage.SHARED))
    targetMachines.set(listOf(machines.windows.x86, machines.windows.x86_64))

    dependencies {
        implementation("at.o2xfs:o2xfs-common-bin:1.0-SNAPSHOT") {
            attributes {
                attribute(Attribute.of("org.gradle.native.linkage", Linkage::class.java), Linkage.STATIC)
            }
        }
        implementation("eu.cen:cen-xfs-api:3.30")
        implementation("eu.cen:cen-xfs-conf:3.30")
        implementation("eu.cen:cen-xfs-supp:3.30")
    }
}

tasks.withType(CppCompile::class.java).configureEach {
    macros.put("_UNICODE", null)
    macros.put("UNICODE", null)

    val javaHome = System.getenv("JAVA_HOME") ?: System.getProperty("java.home")
    includes.from(files("$javaHome/include", "$javaHome/include/win32"))
}

tasks.withType(LinkSharedLibrary::class.java).configureEach {
    linkerArgs.addAll("/MD", "/DEF:o2xfs-spi.def", "Advapi32.lib")
}

publishing {
    repositories {
        maven {
            url = if((version as String).endsWith("-SNAPSHOT")) uri("https://repo.fagschlunger.co.at/libs-snapshot-local") else uri("https://repo.fagschlunger.co.at/libs-release-local")
            credentials {
                val maven_username: String? by project
                val maven_password: String? by project
                username = maven_username
                password = maven_password
            }
        }
    }
}

