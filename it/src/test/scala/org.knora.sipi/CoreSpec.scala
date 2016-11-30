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

import java.util.Date
import java.io.File

import akka.actor.ActorSystem
import akka.event.LoggingAdapter
import akka.http.scaladsl.testkit.ScalatestRouteTest
import org.scalatest.{BeforeAndAfterAll, Matchers, Suite, WordSpecLike}

import scala.io.Source
import scala.sys.process._


/**
  * Core spec class, which is going to be extended by every spec.
  */
class CoreSpec extends Suite with ScalatestRouteTest with WordSpecLike with Matchers with BeforeAndAfterAll {

    private val SipiCommand = "build/sipi -config it/config/sipi.test-config.lua"
    private val SipiWorkingDir = "/Users/subotic/_github.com/sipi" // FIXME
    private val SipiOutputToWaitFor = "Sipi Version"
    private val SipiStartWaitMillis = 5000

    protected def actorRefFactory: ActorSystem = system
    protected val logger: LoggingAdapter = akka.event.Logging(system, this.getClass)
    protected val log: LoggingAdapter = logger

    private var maybeSipiProcess: Option[Process] = None
    private var sipiStarted = false
    private var sipiStartTime: Long = new Date().getTime
    private var sipiTookTooLong = false

    def isAlive: Boolean = {
        if (sipiStarted) {
            maybeSipiProcess match {
                case Some(sipiProcess) => sipiProcess.isAlive
                case None => false
            }
        } else {
            false
        }
    }

    override def beforeAll(): Unit = {
        startSipi()
    }

    override def afterAll(): Unit = {
        maybeSipiProcess match {
            case Some(sipiProcess) => sipiProcess.destroy()
            case None => ()
        }
    }

    /**
      * Starts Sipi and waits for it to finish starting up.
      */
    private def startSipi(): Unit = {
        // Make a ProcessLogger that can monitor Sipi's output to determine whether it has finished starting up.
        val processLogger = ProcessLogger({
            line =>
                if (!sipiStarted) {
                    println(line)

                    if (line.contains(SipiOutputToWaitFor)) {
                        sipiStarted = true
                    }
                }
        })

        // Start the Sipi process.
        val sipiProcess: Process = Process(SipiCommand, new File(SipiWorkingDir)).run(processLogger)
        sipiStartTime = new Date().getTime

        // Wait for Sipi to finish starting up.
        while (!(sipiStarted || sipiTookTooLong)) {
            Thread.sleep(200)

            if (new Date().getTime - sipiStartTime > SipiStartWaitMillis) {
                sipiTookTooLong = true
            }
        }

        require(!sipiTookTooLong, s"Sipi didn't start after $SipiStartWaitMillis ms")
        maybeSipiProcess = Some(sipiProcess)
    }
}
