/*
 * Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
 * Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Sipi.
 * Sipi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Sipi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Additional permission under GNU AGPL version 3 section 7:
 * If you modify this Program, or any covered work, by linking or combining
 * it with Kakadu (or a modified version of that library) or Adobe ICC Color
 * Profiles (or a modified version of that library) or both, containing parts
 * covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
 * or both, the licensors of this Program grant you additional permission
 * to convey the resulting work.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>

#include "shttps/cJSON.h"

#include "SipiError.h"
#include "PhpSession.h"

/*
userdata|
a:12:{
  s:4:"lang"; => s:2:"de";
  s:11:"language_id"; => s:1:"1";
  s:10:"PHPSESSION"; => s:4:"TRUE";
  s:7:"user_id"; => s:1:"5";
  s:5:"token"; => s:22:"b^?¥,^KE^S^Dìpµ@.Á¨»";
  s:8:"username"; => s:4:"root";
  s:9:"firstname"; => s:13:"Administrator";
  s:8:"lastname"; => s:5:"Admin";
  s:5:"email"; => s:27:"lukas.rosenthaler@unibas.ch";
  s:14:"active_project"; => i:1;
  s:8:"projects"; => a:8:{
    i:1;   => s:5:"65807";
    i:6;   => s:5:"65807";
    i:10;  => s:5:"65807";
    i:8;   => s:5:"65807";
    i:13;  => s:5:"65807";
    i:11;  => s:5:"65807";
    i:17;  => s:5:"65807";
    i:19;  => s:5:"65807";}
  s:13:"projects_info"; => a:8:{
    i:0;  => O:8:"stdClass":3:{
      s:2:"id"; -> s:1:"1";
      s:9:"shortname"; -> s:10:"incunabula";
      s:8:"longname"; -> s:22:"Incunabula Basiliensia";
    }
    i:1; => O:8:"stdClass":3:{
      s:2:"id"; -> s:1:"6";
      s:9:"shortname"; -> s:6:"webern";
      s:8:"longname"; -> s:26:"Anton Webern Gesamtausgabe";
    }
    i:2; => O:8:"stdClass":3:{
      s:2:"id"; ->s:2:"10";
      s:9:"shortname"; ->s:3:"SGV";
      s:8:"longname"; -> s:62:"Bilddatenbank der Schweizerischen Gesellschaft für Volkskunde";
    }
    i:3; => O:8:"stdClass":3:{
      s:2:"id"; -> s:1:"8";
      s:9:"shortname"; -> s:7:"dokubib";
      s:8:"longname"; -> s:35:"Bilddatenbank Bibliothek St. Moritz";
    }
    i:4; => O:8:"stdClass":3:{
      s:2:"id"; -> s:2:"13";
      s:9:"shortname"; -> s:6:"salsah";
      s:8:"longname"; -> s:6:"SALSAH";
    }
    i:5; => O:8:"stdClass":3:{
      s:2:"id"; -> s:2:"11";
      s:9:"shortname"; -> s:11:"vitrocentre";
      s:8:"longname"; -> s:18:"Vitrocentre Romont";
    }
    i:6; => O:8:"stdClass":3:{
      s:2:"id"; -> s:2:"17";
      s:9:"shortname"; -> s:9:"neuchatel";
      s:8:"longname"; -> s:19:"Archives Neuchâtel";
    }
    i:7; => O:8:"stdClass":3:{
      s:2:"id"; -> s:2:"19";
      s:9:"shortname"; -> s:9:"parzival1";
      s:8:"longname"; s:12:"Parzival (I)";
    }
  }
}
*/

using namespace std;

static const char __file__[] = __FILE__;

namespace Sipi {

    class PhpException : public std::exception {
    private:
        std::string msg;
        int line;
    public:
        inline PhpException(std::string &msg_p, int line_p) : msg(msg_p), line(line_p) {}
        inline PhpException(const char *msg_p, int line_p) { msg = string(msg_p); line = line_p; }
        inline virtual const char* what() const throw() { return msg.c_str(); }
        inline std::string get(void) { return msg + " #" + to_string(line); }
    };

    static char next (ifstream &in) {
        int i = in.get();
        if (i == EOF) throw PhpException("Unexpected EOF!", __LINE__);
        return (char) i;
    }

    static int get_num (ifstream &in) {
        ostringstream numstr;
        char c;
        while (((c = next(in)) != ':') && (c != ';')) {
            numstr << c;
        }
        return stoi(numstr.str());
    }

    static cJSON *eat_element(ifstream &in) {
        char c, vtype, sep;
        int len;
        vtype = next(in);
        if (vtype == 'N') {
            if ((sep = next(in)) != ';') throw PhpException("Separator expected (\":\")!", __LINE__);
            return cJSON_CreateNull();
        }
        if ((sep = next(in)) != ':') throw PhpException("Separator expected (\":\")!", __LINE__);
        switch (vtype) {
            case 's': {
                len = get_num(in);
                if ((sep = next(in)) != '"') throw PhpException("String start expected ('\"')!", __LINE__);
                ostringstream s;
                for (int i = 0; i < len; i++) s << next(in);
                if ((sep = next(in)) != '"') throw PhpException("String end expected ('\"')!", __LINE__);
                if ((sep = next(in)) != ';') throw PhpException("Value terminator expected (\";\")!", __LINE__);
                return cJSON_CreateString(s.str().c_str());
            }
            case 'i': {
                ostringstream ii;
                while ((c = next(in)) != ';') {
                    ii << c;
                }

                return cJSON_CreateNumber(stoi(ii.str()));
            }
            case 'b': {
                c = next(in);
                if ((sep = next(in)) != ';') throw PhpException("Value terminator expected (\";\")!", __LINE__);
                return cJSON_CreateBool(c);
            }
            case 'O': {
                len = get_num(in);
                if ((sep = next(in)) != '"') throw PhpException("String start expected ('\"')!", __LINE__);
                ostringstream cname;
                for (int i = 0; i < len; i++) cname << next(in);
                if ((sep = next(in)) != '"') throw PhpException("String end expected ('\"')!", __LINE__);
                if ((sep = next(in)) != ':') throw PhpException("Separator expected (\":\")!", __LINE__);

                cJSON *obj = cJSON_CreateObject();

                int num_items = get_num(in);
                if ((sep = next(in)) != '{') throw PhpException("Object start expected (\"{\")!", __LINE__);
                for (int i = 0; i < num_items; i++) {
                    if ((c = next(in)) != 's') throw PhpException("String definition expected (\"{\")!", __LINE__);
                    if ((sep = next(in)) != ':') throw PhpException("Separator expected (\":\")!", __LINE__);
                    len = get_num(in);
                    if ((sep = next(in)) != '"') throw PhpException("String start expected ('\"')!", __LINE__);
                    ostringstream key;
                    for (int i = 0; i < len; i++) key << next(in);
                    if ((sep = next(in)) != '"') throw PhpException("String end expected ('\"')!", __LINE__);
                    if ((sep = next(in)) != ';') throw PhpException("Value terminator expected (\";\")!", __LINE__);
                    cJSON *value = eat_element(in);
                    cJSON_AddItemToObject(obj, key.str().c_str(), value);
                }
                if ((sep = next(in)) != '}') throw PhpException("Object end expected (\"}\")!", __LINE__);

                return obj;
            }
            case 'a': {
                len = get_num(in);
                cJSON *obj = cJSON_CreateObject();

                if ((sep = next(in)) != '{') throw PhpException("Array start expected (\"{\")!", __LINE__);
                for (int i = 0; i < len; i++) {
                    c = next(in);
                    if ((sep = next(in)) != ':') throw PhpException("Separator expected (\":\")!", __LINE__);
                    ostringstream key;
                    switch (c) {
                        case 'i': {
                            int ii = get_num(in);
                            key << ii;
                            break;
                        }
                        case 's': {
                            int len2 = get_num(in);
                            if ((sep = next(in)) != '"') throw PhpException("String start expected ('\"')!", __LINE__);
                            for (int i = 0; i < len2; i++) key << next(in);
                            if ((sep = next(in)) != '"') throw PhpException("String end expected ('\"')!", __LINE__);
                            if ((sep = next(in)) != ';') throw PhpException("Value terminator expected (\";\")!", __LINE__);

                            break;
                        }
                        default: {
                            throw PhpException("Unexpected data type for array key!", __LINE__);
                        }
                    }
                    cJSON *value = eat_element(in);
                    cJSON_AddItemToObject(obj, key.str().c_str(), value);
                }
                if ((sep = next(in)) != '}') throw PhpException("Object end expected (\"}\")!", __LINE__);
                return obj;
            }
        } // switch
        return NULL;
    }

    PhpSession::PhpSession(ifstream *inf_p) {
        lang_id = -1;
        user_id = -1;
        active_project = -1;

        //inf = std::move(inf_p);
		inf = inf_p;
        char c;
        ostringstream name;
        while ((c = next(*inf)) != '|') {
            name << c;
        }
        string objname = name.str();

        cJSON *session;
        try {
            session = eat_element(*inf);
        }
        catch (PhpException &e) {
//            cJSON_Delete(session);
            throw new SipiError(__file__, __LINE__, "Could not get PHP session data for \"" + objname + "\" : " + e.get());
        }

        cJSON *obj;

        if ((obj = cJSON_GetObjectItem(session, "lang")) != NULL) {
            lang = string(obj->valuestring);
        }

        if ((obj = cJSON_GetObjectItem(session, "language_id")) != NULL) {
            lang_id = obj->valueint;
        }


        if ((obj = cJSON_GetObjectItem(session, "user_id")) != NULL) {
            user_id = obj->valueint;
        }

        if ((obj = cJSON_GetObjectItem(session, "active_project")) != NULL) {
            active_project = obj->valueint;
        }

        cJSON_Delete(session);
    }

}
