<?php
// $Id: project_release_table_plugin_style.inc,v 1.2 2010/07/08 23:16:17 dww Exp $

/**
 * @file
 * Class for the project_release_table Views style plugin.
 *
 * We mostly re-use all of the code of the core Views table style, but
 * by having our own style plugin, we can inject our own custom classes into
 * the table HTML to style the rows by release update status.
 *
 */

class project_release_table_plugin_style extends views_plugin_style_table {
}

