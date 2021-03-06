/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013

    .---.           _....._
   /  p  `\     .-""`:     :`"-.
   |__   - |  ,'     .     '    ',
    ._>    \ /:      :     ;      :,
     '-.    '\`.     .     :     '  \
        `.   | .'._.' '._.' '._.'.  |
          `;-\.   :     :     '   '/,__,
          .-'`'._ '     .     : _.'.__.'
         ((((-'/ `";--..:..--;"` \
             .'   /           \   \
       jgs  ((((-'           ((((-'

       The ASCII drawing is Copyright (C) Joan G. Stark
 */

#include <string.h>
#include "version.h"

/* loads an embedded page into a memory buffer and returns */
int load_embedded_page(char *buffer, char *selector)
{
    static const char *welcome =
        "i\n"
        "i          ***  Gopherus v" VERSION " Copyright (C) Mateusz Viste " DATE "  ***\n"
        "i                                           .---.           _....._\n"
        "i Welcome to Gopherus, a multiplatform,    /  p  `\\     .-\"\"`:     :`\"-.\n"
        "i console-mode gopher client.              |__   - |  ,'     .     '    ',\n"
        "i                                           ._>    \\ /:      :     ;      :,\n"
        "i You'll find below a few gopher links that  '-.    '\\`.     .     :     '  \\\n"
        "i might help you start your journey in the      `.   | .'._.' '._.' '._.'.  |\n"
        "i gopher world, as well as a few documents        `;-\\.   :     :     '   '/,__,\n"
        "i related to Gopherus.                            .-'`'._ '     .     : _.'.__.'\n"
        "i If you wish to go to a location you already    ((((-'/ `\";--..:..--;\"` \\\n"
        "i know, enter its URL into the URL bar above         .'   /       jgs \\   \\\n"
        "i (press TAB to switch to the URL bar).             ((((-'           ((((-'\n"
        "i\n"
        "1Veronica - A gopher search engine\t/v2\tgopher.floodgap.com\t70\n"
        "1Super Dimension Fortress (non-profit gopher hosting)\t\tsdf.org\t70\n"
        "1The Online Book Initiative\t1/The Online Book Initiative\tgopher.std.com\t70\n"
        "1The Gopherus home gopher hole\t/projects/gopherus\tgopher.viste-family.net\t70\n"
        "i\n"
        "iGopherus (offline) documentation:\n"
        "0The Gopherus manual\t\t#manual\t70\n"
        "0Read the Gopherus licensing rules (GNU GPL v3)\t\t#license\t70\n";

    static const char *license =
        "\n"
        " Gopherus v" VERSION " Copyright (C) Mateusz Viste " DATE "\n"
        "\n"
        " This program is free software: you can redistribute it and/or modify it under\n"
        " the terms of the GNU General Public License as published by the Free Software\n"
        " Foundation, either version 3 of the License, or (at your option) any later\n"
        " version.\n"
        "\n"
        " This program is distributed in the hope that it will be useful, but WITHOUT\n"
        " ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS\n"
        " FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n"
        "\n"
        " You should have received a copy of the GNU General Public License along with\n"
        " this program. If not, see <http://www.gnu.org/licenses/>.";

    static const char *manual =
        "\n"
        " Gopherus v" VERSION " Copyright (C) Mateusz Viste " DATE "\n"
        "\n"
        "\n"
        " ** Key bindings **\n"
        "\n"
        " Gopherus is a console-mode gopher client. It's entirely keyboard-driven, thus\n"
        " knowing its key bindings is mandatory for comfortable gopher browsing.\n"
        "\n"
        " Key bindings:\n"
        "   TAB       - Switch to/from URL bar edition\n"
        "   ESC       - Quit Gopherus (requires a confirmation)\n"
        "   UP/DOWN   - Scroll the screen's content up/down by one line\n"
        "   PGUP/PGDW - Scroll the screen's content up/down by one page\n"
        "   HOME/END  - Go to the begin/end of the document\n"
        "   BACKSPC   - Go back to the previous location\n"
        "   F1        - Show help (this manual)\n"
        "   F5        - Refresh current location\n"
        "   F9        - Download location on disk\n"
        "\n"
        "\n"
        " ** Customizing the color scheme **\n"
        "\n"
        " The Gopherus color scheme can be customized, if the default one is not suiting\n"
        " you. To customize the color scheme of Gopherus, you will have to set an\n"
        " environnment variable 'GOPHERUSCOLOR' prior to Gopherus launching. This\n"
        " variable have to contain exacty 6 color attributes. Each attribute is\n"
        " describing the foreground/background color for a given element of the Gopherus\n"
        " user interface. An attribute is composed from two hexadecimal digits: XY,\n"
        " where X is the background color, and Y is the foreground color. Colors are\n"
        " indexed as in the classic CGA palette, that is:\n"
        "\n"
        " Index - Color\n"
        "   0   -  black\n"
        "   1   -  low blue\n"
        "   2   -  low green\n"
        "   3   -  low cyan\n"
        "   4   -  low red\n"
        "   5   -  low magenta\n"
        "   6   -  low brown\n"
        "   7   -  light gray\n"
        "   8   -  dark gray\n"
        "   9   -  high blue\n"
        "   A   -  high green\n"
        "   B   -  high cyan\n"
        "   C   -  high red\n"
        "   D   -  high magenta\n"
        "   E   -  yellow\n"
        "   F   -  high intensity white\n"
        "\n"
        " GOPHERUSCOLOR=aabbccddeeffgghhii\n"
        "                | | | | | | | | |\n"
        "                | | | | | | | | |\n"
        "                | | | | | | | | +- Selected item in menu\n"
        "                | | | | | | | |\n"
        "                | | | | | | | +--- Selectable items in menu\n"
        "                | | | | | | |\n"
        "                | | | | | | +----- Error item in menu\n"
        "                | | | | | |\n"
        "                | | | | | +------- Itemtype column in menus\n"
        "                | | | | |\n"
        "                | | | | +--------- URL bar side decorations\n"
        "                | | | |\n"
        "                | | | +----------- URL bar\n"
        "                | | |\n"
        "                | | +------------- Status bar (warning)\n"
        "                | |\n"
        "                | +--------------- Status bar (information)\n"
        "                |\n"
        "                +----------------- Normal text (text files or menu 'i' items)\n"
        "\n"
        " Example:\n"
        "  The default palette used by Gopherus is.......: \"177047707818141220\"\n"
        "  For a black & white mode, use.................: \"077070707808070770\"\n"
        "  Missing those green 1980-like phosphor CRTs?..: \"022020202002020220\"\n"
        "\n"
        "\n"
        " ** Final notes **\n"
        "\n"
        " Gopherus has been written with care to behave nicely and follow standards.\n"
        " It conforms closely to following guidelines:\n"
        "   RFC 1436: The Internet Gopher Protocol\n"
        "   RFC 4266: The gopher URI Scheme\n";

    const char *page;
    size_t len;

    switch (selector[0]) {
        case 'l': /* license */
            page = license;
            break;
        case 'm': /* manual */
            page = manual;
            break;
        default: /* welcome screen */
            page = welcome;
    }

    len = strlen(page);
    memcpy(buffer, page, len);

    return len;
}
