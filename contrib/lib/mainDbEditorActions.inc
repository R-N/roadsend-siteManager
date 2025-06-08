<?php

/*********************************************************************
*  Roadsend SiteManager
*  Copyright (c) 2001-2003 Roadsend, Inc.(http://www.roadsend.com)
**********************************************************************
*
* This source file is subject to version 1.0 of the Roadsend Public
* License, that is bundled with this package in the file 
* LICENSE, and is available through the world wide web at 
* http://www.roadsend.com/license/rpl1.txt
*
**********************************************************************
* Author(s): gollum
*
*
* Change Log
* 
*       10/24/2001 - created
*
*       11/20/2001 - Modified to use the new includeLib function to load
*                    libary file for SM_dbEditorAction
*
*       12/18/2001 - Modified to handle the actual lookup of the data for
*                    for the smart form elements for both the edit and
*                    delete actions. (gollum)
*
*       01/12/2004 - Clean up problems with refrences. Removed old, commented
*                    out code. Changed SM_dbErrorCheck to $this->dbErrorCheck, etc. (gollum)
*
*/

global $SM_siteManager;
$SM_siteManager->includeLib('dbEditorAction');

class addEditorAction extends SM_dbEditorAction {

    /**
     * Method to handle what gets done if the 
     * data on the form is Verified
     */
    
    function dataVerified($myForm) {
        $dbEditor = $this->getDbEditor();
        $database = $dbEditor->dbHL[$dbEditor->directive['dataBaseID']];
        $table = $dbEditor->directive['tableName'];
        $fList = $myForm->getVarList();

        $columns = array();
        $values = array();

        foreach ($fList as $f) {
            if ($f === $dbEditor->directive['fileUpload']) {
                if (empty($this->fileName)) continue;
                $val = $fileName;
            } else {
                $val = $myForm->getVar($f);
            }

            if ($val === '') continue;

            if (($autoDateFields = $this->getDirective('autoDateFields')) && in_array($f, $autoDateFields)) {
                $columns[] = $f;
                $values[] = "NOW()";
                continue;
            }

            if (($autoMD5Fields = $this->getDirective('autoMD5Fields')) && in_array($f, $autoMD5Fields)) {
                $columns[] = $f;
                $values[] = "'" . md5(uniqid(rand(), true)) . "'";
                continue;
            }

            $columns[] = $f;
            $escapedVal = addslashes($val); // Ideally use prepared statements
            $values[] = "'{$escapedVal}'";
        }

        // Handle auto fields not in form
        if (($autoDateFields = $this->getDirective('autoDateFields'))) {
            foreach ($autoDateFields as $f) {
                if (!in_array($f, $fList)) {
                    $columns[] = $f;
                    $values[] = "NOW()";
                }
            }
        }

        if (($autoMD5Fields = $this->getDirective('autoMD5Fields'))) {
            foreach ($autoMD5Fields as $f) {
                if (!in_array($f, $fList)) {
                    $columns[] = $f;
                    $values[] = "'" . md5(uniqid(rand(), true)) . "'";
                }
            }
        }

        $quotedColumns = array_map(function($col) {
            return '"' . $col . '"';
        }, $columns);
        $colList = implode(', ', $quotedColumns);
        $valList = implode(', ', $values);

        // Detect DB type (you may need to adapt this to your framework)
        $dbType = $this->siteConfig->getVar('db','dbType');

        if ($dbType === 'pgsql') {
            $SQL = "INSERT INTO {$table} ({$colList}) VALUES ({$valList}) RETURNING \"idxNum\"";
            $rh = $database->query($SQL);
            $this->dbErrorCheck($rh);
            $row = $rh->fetch(PDO::FETCH_ASSOC);
            return isset($row['id']) ? $row['id'] : null;
        } else { // assume MySQL
            $SQL = "INSERT INTO {$table} ({$colList}) VALUES ({$valList})";
            $rh = $database->query($SQL);
            $this->dbErrorCheck($rh);
            return $database->lastInsertId();
            return mysql_insert_id();
        }
    }

}

class editEditorAction extends SM_dbEditorAction {
    
    /**
     * Method to handle what gets done if the 
     * data on the form is Verified
     */
    function dataVerified($myForm) {
        $dbEditor = $this->getDbEditor();
        $database = $dbEditor->dbHL[$dbEditor->directive['dataBaseID']];

        $SQL = "UPDATE {$dbEditor->directive['tableName']} SET ";
            
        // loop through the variables in the form and update the database
        $fList = $myForm->getVarHash();

        // build the update query               
        foreach ($fList as $f => $fVal) {

            // set the value for file uploads to the file name
            if ($f == $dbEditor->directive['fileUpload']) {

                // Skip on blank - so they don't have to reload the image
                if (empty($this->fileName))
                    continue;

                $val = $fileName;

            // pull the normal value
            } else {
                $val = $fVal;
            }

            // handle enums/checkboxes
            if (is_array($val)) {
                $val = join(',',$val);
            }

            $data = $database->quote($val);

            // append to SQL query  
            $SQL .= "$f=$data, ";  
        }

        // chop off that final comma
        $SQL = substr($SQL, 0, -2);

        // add where clause if editing
        $SQL .= " WHERE {$dbEditor->directive['dataField']}='" . $dbEditor->rNum."'";

        // execute the query
        $rh = $database->query($SQL);
        $this->dbErrorCheck($rh);

        // return the rNum
        return $dbEditor->rNum;

    }

    /**
     * Method to handle anything that needs to 
     * be done before the form is actually run.
     */
    function preFormRun($myForm) {
        
        $dbEditor = $this->getDbEditor();

        $this->debugLog('rNum is '.$this->directive['rNumVar'].' and its value is '.$dbEditor->rNum);

        // if we've got a record we're viewing, get the data from the databse
        if ($dbEditor->rNum != 0) {
            
            $SQL = "SELECT * FROM {$dbEditor->directive['tableName']} WHERE {$dbEditor->directive['dataField']}='" . $dbEditor->rNum."'";
            $rh = $dbEditor->dbHL[$dbEditor->directive['dataBaseID']]->query($SQL);
            $this->dbErrorCheck($rh);            
            $rr = $rh->fetch();
            $dbEditor->rowData = $rr;
        }

        // ge a list of vars from the form and attempt to set the default values
        $list = $myForm->getVarList();
        foreach($list as $name) {
            // make sure it's not on the noDbValue list
            if ( (!isset($this->dbEditor->directive['noDbValue'])) || ((is_array($this->dbEditor->directive['noDbValue'])) && 
                 (!in_array($name, $this->dbEditor->directive['noDbValue']))) ) {
                    $myForm->setDefaultvalue($name, $rr[$name]);
                 }
        }

    }

}

class removeEditorAction extends SM_dbEditorAction {

    /**
     * Method to handle what gets done if the 
     * data on the form is Verified
     */
    function dataVerified($myForm) {
        $dbEditor = $this->getDbEditor();
        $database = $dbEditor->dbHL[$dbEditor->directive['dataBaseID']];

        // make sure they confirmed
        if ($myForm->getVar('_removeConfirm') != 'yes') {

            // redisplay form
            $myForm->setHeader("YOU MUST CHECK THE BOX TO CONFIRM REMOVAL");
            $sMsg = SM_prettyName($dbEditor->sType);
            $dbEditor->say($myForm->output($sMsg,array($this->directive['actionVar']=>$dbEditor->sType,$this->directive['rNumVar']=>$dbEditor->rNum)));
            return;
        }
        
        // support for external remove function
        if (method_exists($dbEditor->directive['removeObject'],$dbEditor->directive['removeFunction'])) {
            $object = $dbEditor->directive['removeObject'];
            $fName = $dbEditor->directive['removeFunction'];
            $object->$fName($dbEditor->rNum);
        }

        // verified, remove it
        $database->query("DELETE FROM {$dbEditor->directive['tableName']} WHERE {$dbEditor->directive['dataField']}='" . $dbEditor->rNum."'");

        //return 0 because everything went ok.
        return 0;
    }

    /**
     * Method to handle anything that needs to 
     * be done before the form is actually run.
     */
    function preFormRun($myForm) {

        $dbEditor= $this->getDbEditor();

        // if we've got a record we're viewing, get the data from the databse
        if ($dbEditor->rNum != 0) {
            $SQL = "SELECT * FROM {$dbEditor->directive['tableName']} WHERE {$dbEditor->directive['dataField']}='" . $dbEditor->rNum."'";
            $rh = $dbEditor->dbHL[$dbEditor->directive['dataBaseID']]->query($SQL);
            $this->dbErrorCheck($rh, $this);
            
            $rr = $rh->fetch();
        }

        // ge a list of vars from the form and attempt to set the default values
        $list = $myForm->getVarList();
        foreach($list as $name) {
            $myForm->setDefaultvalue($name, $rr[$name]);
        }

        // add an extra check so that we can confirm the delete.
        if($dbEditor->sType == $this->getTypeName()) {
            $myForm->addDirective('requiredTag','');
            $rC = $myForm->add('_removeConfirm','','checkBox',true);
            $rC->addOption('Are you SURE you wish to remove this item?','yes');
            //$myForm->setLayout('_removeConfirm',SF_LAYOUT_SINGLE);
        }
    }


}


?>
