/*
 * Copyright © 2015 Lukas Rosenthaler, Benjamin Geer, Ivan Subotic,
 * Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Knora.
 * Knora is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Knora is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Knora.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.knora.sipi

import akka.actor.ActorSystem
import akka.http.scaladsl.Http
import akka.http.scaladsl.model._
import akka.http.scaladsl.testkit.RouteTestTimeout

import scala.concurrent.Await
import scala.concurrent.duration.DurationInt

/**
  * This Spec holds examples on how certain akka features can be used.
  */
class BasicSpec extends CoreSpec {

    implicit def default(implicit system: ActorSystem) = RouteTestTimeout(new DurationInt(15).second)

    "Sipi" should {

        "return an image using the simulated Knora" in {
            val responseFuture = Http().singleRequest(HttpRequest(uri = s"$sipiBaseUrl/knora/incunabula_0000003846.jpg/full/full/0/default.jpg"))
            val response: HttpResponse = Await.result(responseFuture, 3.seconds)
            assert(response.status === StatusCodes.OK)
        }

    }
}
