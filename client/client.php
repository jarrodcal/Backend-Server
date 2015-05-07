<?php

    $uids = array(1984467097, 1111681197);
    
    foreach ($uids as $uid)
    {
        $begin = microtime(true);

        $ctx = stream_context_create();
        $socket = stream_socket_client('tcp://172.16.38.26:9432', $err, $errstr, 1, STREAM_CLIENT_CONNECT, $ctx);

        if ($socket === false)
        {
            echo "Connect Server Error\n";
            continue;
        }
        
        $len = strlen($uid);
        $message = '$' . "{$len}\r\n{$uid}";

        stream_socket_sendto($socket, $message);
        $val = stream_socket_recvfrom($socket, 100);

        stream_socket_shutdown($socket);
        $end = microtime(true);
        $used = $end - $begin;

        echo $val . ' used ' . $used . "\n";
    }