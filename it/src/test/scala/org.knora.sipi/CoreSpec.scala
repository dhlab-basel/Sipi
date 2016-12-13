/*
 * Copyright © 2015 Lukas Rosenthaler, Benjamin Geer, Ivan Subotic,
 * Tobias Schweizer, André Kilchenmann, and André Fatton.
 *
 * This file is part of Knora.
 *
 * Knora is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Knora is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with Knora.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.knora.sipi

import java.io.File
import java.util.Date

import akka.actor.ActorSystem
import akka.event.LoggingAdapter
import akka.http.scaladsl.Http
import akka.http.scaladsl.model.{HttpRequest, HttpResponse, StatusCodes}
import akka.http.scaladsl.testkit.ScalatestRouteTest
import com.typesafe.config.{Config, ConfigFactory}
import org.apache.commons.io.input.{Tailer, TailerListener, TailerListenerAdapter}
import org.scalatest._

import scala.concurrent.Await
import scala.sys.process._
import scala.concurrent.duration.DurationInt


/**
  * Core spec class, which is going to be extended by every spec.
  */
abstract class CoreSpec extends Suite with ScalatestRouteTest with WordSpecLike with Matchers with BeforeAndAfterAll with BeforeAndAfterEach {
    protected def actorRefFactory: ActorSystem = system
    protected val log: LoggingAdapter = akka.event.Logging(system, this.getClass)

    protected val config: Config = ConfigFactory.load()
    protected val sipiBaseUrl: String = config.getString("test.sipi.base-url")

    private val sipiConfigFile: String = config.getString("test.sipi.config-file")
    private val sipiReadyOutput = config.getString("test.sipi.ready-output")
    private val sipiStartWaitMillis = config.getDuration("test.sipi.start-wait").toMillis
    private val sipiLogFileDelayMillis = config.getDuration("test.sipi.log-file-delay").toMillis
    private val nginxBaseUrl = config.getString("test.nginx.base-url")

    private val sipiCommand = s"local/bin/sipi -config config/$sipiConfigFile"
    private val SipiLogFile = new File("../sipi.log")
    private val SipiWorkingDir = new java.io.File("..").getCanonicalPath

    private val NginxWorkingDir = new java.io.File("../it/src/test/resources/nginx/")
    private val StartNginxCommand = s"nginx -p ${NginxWorkingDir.getAbsolutePath} -c nginx.conf"
    private val StopNginxCommand = s"nginx -p ${NginxWorkingDir.getAbsolutePath} -c nginx.conf -s stop"

    private var maybeSipiProcess: Option[Process] = None
    private var sipiStarted = false
    private var sipiStartTime: Long = new Date().getTime
    private var sipiTookTooLong = false

    /**
      * A [[TailerListener]] for getting the output that Sipi sends to its log file.
      */
    private class SipiTailerListener extends TailerListenerAdapter {
        private val sipiLogFileOutput = new StringBuilder

        override def handle(line: String): Unit = {
            sipiLogFileOutput.append(line).append("\n")
        }

        def getLogFileOutput: String = sipiLogFileOutput.toString
    }

    private val tailerListener = new SipiTailerListener

    private val tailer = Tailer.create(SipiLogFile, tailerListener, sipiLogFileDelayMillis)

    def sipiIsRunning: Boolean = {
        if (sipiStarted) {
            maybeSipiProcess match {
                case Some(sipiProcess) => sipiProcess.isAlive
                case None => false
            }
        } else {
            false
        }
    }

    override def beforeEach(): Unit = {
        assert(sipiIsRunning)
    }

    override def beforeAll(): Unit = {
        startNginx()
        startSipi()
    }

    override def afterAll(): Unit = {
        maybeSipiProcess match {
            case Some(sipiProcess) => sipiProcess.destroy()
            case None => ()
        }

        tailer.stop()
        stopNginx()
    }

    def getSipiLogOutput: String = {
        tailerListener.getLogFileOutput
    }

    private def startNginx(): Unit = {
        new File(NginxWorkingDir, "logs").mkdirs()
        assert(Process(StartNginxCommand, NginxWorkingDir).run().exitValue == 0, "nginx failed to start")
        val responseFuture = Http().singleRequest(HttpRequest(uri = nginxBaseUrl))
        val response: HttpResponse = Await.result(responseFuture, 3.seconds)
        log.debug(s"Response from nginx: ${response.toString}")
        assert(response.status === StatusCodes.OK, "nginx did not respond")
    }

    private def stopNginx(): Unit = {
        assert(Process(StopNginxCommand, NginxWorkingDir).run().exitValue == 0, "nginx failed to stop")
    }

    /**
      * Starts Sipi and waits for it to finish starting up.
      */
    private def startSipi(): Unit = {
        // Make sure Sipi has a cache directory.
        new File(SipiWorkingDir, "cache").mkdirs()

        // Make a ProcessLogger that can monitor Sipi's output to determine whether it has finished starting up.
        val processLogger = ProcessLogger({
            line =>
                if (!sipiStarted) {
                    println(line)

                    if (line.contains(sipiReadyOutput)) {
                        sipiStarted = true
                    }
                }
        })

        // Start the Sipi process.

        // println(s"Sipi command: $sipiCommand")
        val sipiProcess: Process = Process(sipiCommand, new File(SipiWorkingDir)).run(processLogger)
        sipiStartTime = new Date().getTime

        // Wait for Sipi to finish starting up.
        while (!(sipiStarted || sipiTookTooLong)) {
            Thread.sleep(200)

            if (new Date().getTime - sipiStartTime > sipiStartWaitMillis) {
                sipiTookTooLong = true
            }
        }

        require(!sipiTookTooLong, s"Sipi didn't start after $sipiStartWaitMillis ms")
        maybeSipiProcess = Some(sipiProcess)
    }
}
