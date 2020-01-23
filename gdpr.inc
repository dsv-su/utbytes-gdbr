<?php

function gdpr_export($string) {
	/*
	if (php_sapi_name() == 'cli') {
		$opts = "o:p:e:";
		$input = getopt($opts);
		$op = $input['o'];
		$pn = $input['p'];
		$email = $input['e'];
	} else {
		//op: export = 1, delete = 2
		//$op = $_GET['op'];
		//$pn = $_GET['pn'];
		//$email = $_GET['mail'];
	}*/

	if (empty($string)) {
		http_response_code(400);
		exit;
	}

	parse_str($string, $arr);
	$email = $arr['email'] ?? null;
	$pn = $arr['pn'] ?? null;
	$path = DRUPAL_ROOT.'/sites/default/files/tmp/';

	if (!$email && !$pn) {
		http_response_code(204);
	}

	delTree($path);
	mkdir($path, 0777);

	$types = array('incoming', 'outgoing');

	$allempty = true;

	foreach ($types as $type) {
		$nodes = node_load_multiple(array(), array('type' => $type));
		$nodes_email = $email ? array_filter($nodes, function ($n) use ($email) {
			return (trim($n->field_email_['und'][0]['email']) == $email);
		}) : array();
		$nodes_pn = $pn ? array_filter($nodes, function ($n) use ($pn) {
			return (trim($n->field_p_number['und'][0]['value']) == $pn ||
				((trim($n->field_p_number['und'][0]['value']) == substr($pn, 2)) && strlen($pn) == 13));
		}) : array();

		if ($email && $pn && (array_values($nodes_email) !== array_values($nodes_pn))) {
			// The requested user could not be found or credentials point to different users.
			http_response_code(204);
			die();
		}

		$nodes = $nodes_email + $nodes_pn;

		if (empty($nodes)) {
			continue;
		}

		$allempty = false;

		$id = $email ?? $pn;

		$headers = array();
		$rows = array();
		$files = array();

		foreach ($nodes as $nid => $n) {
			foreach ($n as $key => $values) {
				if ($key == 'rdf_mapping' || $key == 'field_country2') {
					continue;
				}
				if (is_array($values)) {
					$info = field_info_instance('node', $key, $type);
					$headers[] = $info['label'];
					$rows[$nid][] = export_field_data($values, $key, $files);
				}
			}
		}

		$file = fopen($path.$id.'-'.$type.'.csv', 'w') or die('Problems');
		fputcsv($file, $headers);
		foreach ($rows as $row) {
			fputcsv($file, $row);
		}
		fclose($file);
	}

	if ($allempty) {
		http_response_code(204);
		exit;
	}

	$zip = new ZipArchive;
	if ($zip->open($path.$id.'.zip', ZipArchive::CREATE) === TRUE) {   
		foreach ($types as $type) {
			$file = $path.$id.'-'.$type.'.csv';
			$new_filename = substr($file, strrpos($file,'/') + 1);
			$zip->addFile($file, $new_filename);
		}
		foreach ($files as $file) {
			$file = str_replace('utbytes/gdpr/', 'utbytes/', $file);
			$new_filename = substr($file, strrpos($file,'/') + 1);
			$current = file_get_contents($file);
			if (!empty($current)) {
				file_put_contents($path.'/'.$new_filename, $current);
			}
			$zip->addFile($path.'/'.$new_filename, $new_filename);
		}
		$zip->close();
		readfile($path.$id.'.zip');
		http_response_code(200);
	} else {
		http_response_code(500);
		die();
	}
}

// Functions

function delTree($dir) {
	$files = array_diff(scandir($dir), array('.','..'));
	foreach ($files as $file) {
		(is_dir("$dir/$file")) ? delTree("$dir/$file") : unlink("$dir/$file");
	}
	return rmdir($dir);
}

function join_list ($l, $attr) {
	$string = '';
	foreach ($l as $i) {
		$string .= $i[$attr].', ';
	}
	return substr($string, 0, -2);
}

function export_field_data ($f, $name, &$files) {
	if (is_array($f)) {
		$f = $f['und'];
		switch ($name) {
			case 'field_courses':
			case 'field_period':
			case 'field_control':
			case 'field_control_out':
				return join_list($f, 'value');
				break;

			case 'field_email_':
			case 'field_partner_email':
				return $f[0]['email'];
				break;

			case 'field_universitytax':
			case 'field_periodtax':
				$term = taxonomy_term_load($f[0]['tid']);
				return $term->name;
				break;

			case 'field_insurance_student_out_vali':
				return $f[0]['value'].' - '.$f[0]['value2'].' ('.$f[0]['timezone'].')';
				break;

			case 'field_files':
			case 'field_file':
			case 'field_foto':
				foreach ($f as $file) {
					$url = file_create_url($file['uri']);
					$files[] = $url;
				}
				return join_list($f, 'filename');
				break;

			default:
				if ($f[0]['value']) {
					return $f[0]['value'];
				}
				break;
		}
	}
}