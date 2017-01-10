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
    scalaVersion := "2.12.1"
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

lazy val akkaVersion = "2.4.14"
lazy val akkaHttpVersion = "10.0.0"

lazy val sipiTestLibs = Seq(
    "org.scalatest" %% "scalatest" % "3.0.0" % "test",
    "com.typesafe.akka" %% "akka-actor" % akkaVersion % "test",
    "com.typesafe.akka" %% "akka-agent" % akkaVersion % "test",
    "com.typesafe.akka" %% "akka-stream" % akkaVersion % "test",
    "com.typesafe.akka" %% "akka-slf4j" % akkaVersion % "test",
    "com.typesafe.akka" %% "akka-http" % akkaHttpVersion % "test",
    "com.typesafe.akka" %% "akka-http-spray-json" % akkaHttpVersion % "test",
    "com.typesafe.akka" %% "akka-testkit" % akkaVersion % "test",
    "com.typesafe.akka" %% "akka-http-testkit" % akkaHttpVersion % "test",
    "com.typesafe.akka" %% "akka-stream-testkit" % akkaVersion % "test",
    "org.scala-lang.modules" %% "scala-xml" % "1.0.6" % "test",
    "commons-io" % "commons-io" % "2.5" % "test"
)

scalacOptions += "-feature"
