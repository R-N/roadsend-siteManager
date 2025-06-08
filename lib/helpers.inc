<?php

if (!function_exists('addSchema')) {
    function addSchema($table){
        $table = trim($table);
        if (strpos($table, ".") !== FALSE || strpos($table, "?") !== FALSE) {
            return $table;
        }
        $schema = "public";
        if ($schema == "public")
            return $table;
        return "\"{$schema}\".{$table}";
    }
}

if (!function_exists('quoteIdentifiers')) {
  function quoteIdentifiers($clause) {
    return preg_replace_callback(
        '/\b([a-z_][a-z0-9_]*)(?:\.([a-z_][a-z0-9_]*))?\b/i',
        function ($m) {
            static $keywords = array(
                // SQL keywords & functions to skip quoting
                'AND','OR','NOT','IN','IS','NULL','LIKE','BETWEEN','EXISTS',
                'TRUE','FALSE','LOWER','UPPER','SUBSTR','TRIM','CAST','COALESCE',
                'CASE','WHEN','THEN','ELSE','END','COUNT','SUM','AVG','MIN','MAX'
            );
            $word = strtoupper($m[0]);

            if (in_array($word, $keywords)) return $m[0];
            if (is_numeric($m[0])) return $m[0];
            if (preg_match('/^["\'].*["\']$/', $m[0])) return $m[0];

            if (!empty($m[2])) {
                return '"' . $m[1] . '"."' . $m[2] . '"';
            }
            return '"' . $m[1] . '"';
        },
        $clause
    );
  }
}

if (!function_exists('mysqlToPostgres')) {
    require_once("ParensParser.php");
    
    function mysqlToPostgres($query, $parsed=0) {

        if (!is_string($query)){
            $main = '';
            $subqueries = array();
            
            $subqueryCounter = 0;
            $i = 0;
            foreach($query as $q){
                // consoleLog("q{$parsed}.{$i}");
                // consoleLog($q);
                if (is_string($q)){
                    $main = trim($main . " " . trim($q));
                }else{
                    $q = mysqlToPostgres($q, $parsed+1);
                    $q = trim($q);
                    array_push($subqueries, $q);
                    $main = trim($main . " (?" . $subqueryCounter++ . ")");
                }
                // consoleLog($q);
            }
            $main = trim($main);
            // consoleLog("Building {$parsed}");
            // consoleLog($main);
            // consoleLog($subqueries);
            $main = mysqlToPostgres($main, $parsed);
            // consoleLog($main);
            $query2 = $main;
            $i = 0;
            foreach ($subqueries as $q){
                $query2 = str_replace('?' . ($i++), $q, $query2);
            }
            // consoleLog($query2);
            $query = $query2;
            if ($parsed == 1){
                return $query;
            }
        }

        $schema = "public";  // Replace with your dynamic schema name

        // Replace backticks
        $query = str_replace('`', '', $query);

        if (!$parsed){
            $parensParser = new ParensParser();
            // consoleLog("Parsing!");
            // consoleLog($query);
            $arr = $parensParser->parse($query);
            // consoleLog($arr);
            if (!(count($arr) == 1 && is_string($arr[0]))){
                $query = mysqlToPostgres($arr, 1);
            }
        }

        // Replace Show Columns
        $query = preg_replace(
            '/SHOW\s+COLUMNS\s+FROM\s+\"?([\`\'\"a-zA-Z0-9\-_,\(\)\?\s]+?)\"?(?=$|\;|\s+(ON|WHERE|HAVING|LIMIT|OFFSET|JOIN|LEFT\s+JOIN|RIGHT\s+JOIN|\;|$))/i', 
            "
            SELECT DISTINCT
                c.column_name AS \"Field\",
                c.data_type AS \"Type\",
                c.is_nullable AS \"Null\",
                CASE 
                    WHEN tc.constraint_type = 'PRIMARY KEY' THEN 'PRI'
                    WHEN tc.constraint_type = 'UNIQUE' THEN 'UNI'
                    WHEN tc.constraint_type = 'FOREIGN KEY' THEN 'MUL'
                    ELSE ''
                END AS \"Key\",
                c.column_default AS \"Default\",
                CASE 
                    WHEN c.is_identity = 'YES' THEN 'auto_increment'
                    ELSE ''
                END AS \"Extra\",
                CASE 
                    WHEN tc.constraint_type = 'PRIMARY KEY' THEN TRUE
                    ELSE FALSE
                END AS \"primary_key\"
            FROM 
                information_schema.columns c
            LEFT JOIN 
                information_schema.key_column_usage kcu 
                ON c.table_schema = kcu.table_schema 
                AND c.table_name = kcu.table_name 
                AND c.column_name = kcu.column_name
            LEFT JOIN 
                information_schema.table_constraints tc 
                ON kcu.constraint_name = tc.constraint_name
            WHERE 
                c.table_schema = '{$schema}' 
                AND c.table_name = '$1';
            ", 
            $query
        );


        // Wrap select column name in quotation marks
        $query = preg_replace_callback('/\bSELECT\s+(.+?)(?=\bFROM\b)/is', function($matches) {
            $cols = explode(',', $matches[1]);
            foreach ($cols as &$col) {
                $col = trim($col);
                
                // Match aliases
                if (preg_match('/^(.+?)\s+(AS\s+)?("?)([a-zA-Z_][a-zA-Z0-9_]*)\3$/i', $col, $m)) {
                    $expr = quoteIdentifiers(trim($m[1]));
                    $alias = '"' . $m[4] . '"';
                    $col = "{$expr} " . ($m[2] ?: '') . $alias;
                } else {
                    $col = quoteIdentifiers($col);
                }
            }
            return 'SELECT ' . implode(', ', $cols);
        }, $query);

        // Wrap columns in ORDER BY
        $query = preg_replace_callback('/\bORDER\s+BY\s+(.+?)(?=\b(LIMIT|OFFSET|FETCH|\)|$))/is', function($matches) {
            $cols = explode(',', $matches[1]);
            foreach ($cols as &$col) {
                $col = trim($col);
                // Match: col [ASC|DESC]
                if (preg_match('/^(.+?)\s+(ASC|DESC)$/i', $col, $m)) {
                    $col = quoteIdentifiers($m[1]) . ' ' . strtoupper($m[2]);
                } else {
                    $col = quoteIdentifiers($col);
                }
            }
            return 'ORDER BY ' . implode(', ', $cols);
        }, $query);

        //Wrap columns in GROUP BY
        $query = preg_replace_callback('/\bGROUP\s+BY\s+(.+?)(?=\b(HAVING|ORDER|LIMIT|OFFSET|FETCH|\)|$))/is', function ($matches) {
            $cols = explode(',', $matches[1]);
            foreach ($cols as &$col) {
                $col = trim($col);
                $col = quoteIdentifiers($col);
            }
            return 'GROUP BY ' . implode(', ', $cols);
        }, $query);

        //Wrap columns in WHERE
        $query = preg_replace_callback('/\bWHERE\b\s+(.+?)(?=\b(GROUP\s+BY|HAVING|ORDER\s+BY|LIMIT|$))/is', function($m) {
            return 'WHERE ' . quoteIdentifiers($m[1]) . ' ';
        }, $query);
        
        //Wrap columns in HAVING
        $query = preg_replace_callback('/\bHAVING\b\s+(.+?)(?=\b(ORDER\s+BY|LIMIT|$))/is', function($m) {
            return 'HAVING ' . quoteIdentifiers($m[1]) . ' ';
        }, $query);


        // Add schema name to tables in FROM, JOIN, LEFT JOIN, RIGHT JOIN clauses (only once per table name)
        $query = preg_replace_callback('/\b(INSERT\s+INTO|UPDATE|FROM|JOIN|LEFT\s+JOIN|RIGHT\s+JOIN)\s+([\`\'\"a-zA-Z0-9\-_,\(\)\?\s]+?)(?=$|\;|\s+(ON|WHERE|HAVING|LIMIT|OFFSET|JOIN|LEFT\s+JOIN|RIGHT\s+JOIN|ORDER\s+BY|VALUES|SET|\(|\;|$))/i', function($matches) use ($schema) {
            // Split comma-separated tables
            $tables = explode(',', $matches[2]);
            foreach ($tables as &$table) {
                // Trim spaces and add schema to each table name
                $table = addSchema($table);
            }
            $tables = implode(', ', $tables);
            $rest = '';
            //$rest = $matches[3] ? "{$matches[3]} {$rest}" : $rest;
            //$rest = $matches[4] ? "{$matches[4]} {$rest}" : $rest;
            return "{$matches[1]} {$tables} {$rest}";
        }, $query);

        // Remove duplicate schema occurrences
        $query = str_replace("\"{$schema}\".\"{$schema}\"", "\"{$schema}\"", $query);
        $query = str_replace("\"{$schema}\".\"{$schema}\"", "\"{$schema}\"", $query);

        // Replace CURDATE() with CURRENT_DATE
        $query = preg_replace('/\bCURDATE\(\)/i', 'CURRENT_DATE', $query);
        // Replace INTERVAL X UNIT with INTERVAL 'X units', ensuring unit is plural
        $query = preg_replace_callback('/INTERVAL\s+(\d+)\s+(\w+)/i', function($matches) {
            $unit = strtolower($matches[2]);
            // Ensure unit is plural for PostgreSQL
            if (substr($unit, -1) !== 's') {
                $unit .= 's';
            }
            return "INTERVAL '{$matches[1]} $unit'";
        }, $query);
        // Replace YEAR(<column>) with EXTRACT(YEAR FROM <column>)
        $query = preg_replace('/YEAR\(([^)]+)\)/i', 'EXTRACT(YEAR FROM $1)', $query);
        // Replace MONTH(<column>) with EXTRACT(MONTH FROM <column>)
        $query = preg_replace('/MONTH\(([^)]+)\)/i', 'EXTRACT(MONTH FROM $1)', $query);
        // Replace DAYNAME(<column>) with TO_CHAR(<column>, 'Day')
        $query = preg_replace('/DAYNAME\(([^)]+)\)/i', "TO_CHAR($1, 'Day')", $query);
        // Replace WEEKDAY(<column>) with EXTRACT(DOW FROM <column>)
        $query = preg_replace('/WEEKDAY\(([^)]+)\)/i', 'EXTRACT(DOW FROM $1)', $query);


        return $query;
    }
}

?>
