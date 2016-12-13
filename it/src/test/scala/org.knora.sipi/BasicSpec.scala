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

import java.io.File
import java.util

import akka.http.scaladsl.model._

import scala.concurrent.{Await, Future}
import scala.concurrent.duration.DurationInt

/**
  * Tests basic features of Sipi.
  */
class BasicSpec extends CoreSpec {

    "Sipi" should {

        "return an image" in {
            val responseFuture: Future[HttpResponse] = http.singleRequest(HttpRequest(uri = s"$sipiBaseUrl/Leaves.jpg/full/full/0/default.jpg"))
            val response: HttpResponse = Await.result(responseFuture, 10.seconds)
            assert(response.status == StatusCodes.OK)
        }

        "return a JPG file as a JPG containing the correct bytes" in {
            val fileBytes: Array[Byte] = readFileAsBytes(new File(dataDir, "Leaves.jpg"))
            val bytesFromSipi: Array[Byte] = downloadBytes(s"$sipiBaseUrl/Leaves.jpg/full/full/0/default.jpg")
            assert(util.Arrays.equals(fileBytes, bytesFromSipi))
        }
    }
}
