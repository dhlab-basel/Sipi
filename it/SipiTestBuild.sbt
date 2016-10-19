import sbt._
import sbt.Keys._

lazy val SipiTest = (project in file(".")).
        settings(sipiTestCommonSettings:  _*).
        settings(
            libraryDependencies ++= sipiTestLibs,
            logLevel := Level.Info,
            fork in run := true,
            javaOptions in run ++= javaRunOptions,
            fork in Test := true,
            javaOptions in Test ++= javaTestOptions,
            parallelExecution in Test := false,
            /* show full stack traces and test case durations */
            testOptions in Test += Tests.Argument("-oDF"),
            ivyScala := ivyScala.value map { _.copy(overrideScalaVersion = true) }
        )

lazy val sipiTestCommonSettings = Seq(
    organization := "org.knora",
    name := "SipiTest",
    version := "0.1.0",
    scalaVersion := "2.11.8"
)

lazy val javaRunOptions = Seq(
    // "-showversion",
    "-Xms2048m",
    "-Xmx4096m"
    // "-verbose:gc",
    //"-XX:+UseG1GC",
    //"-XX:MaxGCPauseMillis=500"
)

lazy val javaTestOptions = Seq(
    // "-showversion",
    "-Xms2048m",
    "-Xmx4096m"
    // "-verbose:gc",
    //"-XX:+UseG1GC",
    //"-XX:MaxGCPauseMillis=500",
    //"-XX:MaxMetaspaceSize=4096m"
)

lazy val sipiTestLibs = Seq(
    "com.typesafe.akka" %% "akka-http-core" % "2.4.11" % "test",
    "com.typesafe.akka" %% "akka-http-testkit" % "2.4.11" % "test",
    "org.scalatest" %% "scalatest" % "3.0.0" % "test",
    "com.typesafe.akka" %% "akka-slf4j" % "2.4.11" % "test",
    "com.typesafe.akka" %% "akka-stream" % "2.4.11" % "test",
    "com.typesafe.akka" %% "akka-stream-testkit" % "2.4.11" % "test",
    "com.typesafe.akka" %% "akka-http-experimental" % "2.4.11" % "test",
    "com.typesafe.akka" %% "akka-http-spray-json-experimental" % "2.4.11" % "test",
    "com.typesafe.akka" %% "akka-http-xml-experimental" % "2.4.11" % "test"
)