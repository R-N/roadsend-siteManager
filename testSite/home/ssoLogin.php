<?php

// include site configuration file, which includes siteManager libs
require('../admin/common.inc');

// load requested module
$mod = $SM_siteManager->loadModule('ssoLogin');
$mod->addDirective('showLoginStatus',true);
$output = $mod->run();

// create root template. notice, returns a reference!!
$layout1 = $SM_siteManager->rootTemplate("main.cpt");

// add the module to the codePlate
$layout1->addModule($mod, 'main');

// finish display
$SM_siteManager->completePage();

?>