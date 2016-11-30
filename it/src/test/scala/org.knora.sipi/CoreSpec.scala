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

import akka.http.scaladsl.testkit.ScalatestRouteTest
import org.scalatest.{BeforeAndAfterAll, Matchers, Suite, WordSpecLike}

import scala.sys.process._


/**
  * Core spec class, which is going to be extended by every spec.
  */
class CoreSpec extends Suite with ScalatestRouteTest with WordSpecLike with Matchers with BeforeAndAfterAll {

    def actorRefFactory = system
    val logger = akka.event.Logging(system, this.getClass())
    val log = logger

    def isAlive: Boolean = {
        //Check if Sipi is still running
        true
    }

    def tailConsole(lines: Int): String = {
        //Return last number of lines from the console output
        ""
    }

    def tailLog(lines: Int): String = {
        s"tail -n$lines ../sipi.log.txt" !!
    }

    override def beforeAll {
        //Here we should start a fresh sipi instance
        //FIXME: Make it portable
        val sipi = Process("build/sipi -config it/config/sipi.test-config.lua", new java.io.File("/Users/subotic/_github.com/sipi")).lineStream
    }

    override def afterAll {
        //Here we should stop the sipi instance
    }

}
