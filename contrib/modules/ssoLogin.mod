<?php

require_once 'JWT.php';
use \Firebase\JWT\JWT;
class SM_ssoLogin extends SM_module {
    var $keycloakKey = null;
    var $authentikKey = null;

    function moduleConfig() {
        $this->keycloakKey = $this->loadPublicKey('keycloak_key');
        $this->authentikKey = $this->loadPublicKey('authentik_key', false);
    }

    function loadPublicKey($fName, $verify=true){
        global $SM_siteManager;
        $sName = $SM_siteManager->findSMfile($fName, 'keys', 'pem', true);
        $key = file_get_contents($sName);
        if ($verify){
            $resource = openssl_pkey_get_public($key);
            if ($resource === false) {
                // $this->say("Failed to load key: " . $fName);
                return null;
            }
        }
        return $key;
    }

    function canModuleLoad() {
        return $this->sessionH->directive['useMemberSystem'];
    }

    function verifyTokenUrl($accessToken, $verifyUrl){
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $verifyUrl);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_HTTPHEADER, array(
            "Authorization: Bearer " . $accessToken
        ));

        $response = curl_exec($ch);
        $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);

        curl_close($ch);

        if ($http_code == 200) {
            // Token is valid
            try{
                $userinfo = json_decode($response, true);
                return $userinfo;
            } catch (Exception $e) {
                return true;
            }
        } else {
            $this->say("Invalid token ");
            return null;
        }
    }

    function verifyTokenKey($accessToken, $key, $alg=null){
        if (!$alg){
            $alg = "RS256";
        }
        try {
            $decoded = JWT::decode($accessToken, $key, array($alg));
            return $decoded;
        } catch (Exception $e) {
            $this->say("Invalid token: " . $e->getMessage());
            return null;
        }
    }

    function moduleThink() {
        // If already logged in, just redirect
        if (!$this->sessionH->isGuest()) {
            header("Location: /");
            return;
        }

        $accessToken = null;
        $verifyUrl = null;
        $key = null;
        $alg = null;
        if (isset($_SERVER['HTTP_X_FORWARDED_ACCESS_TOKEN'])){
            $accessToken = $_SERVER['HTTP_X_FORWARDED_ACCESS_TOKEN'];
            $verifyUrl = "http://localhost:8080/realms/rsec/protocol/openid-connect/userinfo";
            $key = $this->keycloakKey;
        }else if (isset($_SERVER['HTTP_X_AUTHENTIK_JWT'])){
            $accessToken = $_SERVER['HTTP_X_AUTHENTIK_JWT'];
            // $verifyUrl = "http://localhost:9000/api/v3/outposts/proxy/";
            $key = $this->authentikKey;
            $alg = "HS256"; //UNSAFE
        }

        if ($accessToken){
            if ($key && !$this->verifyTokenKey($accessToken, $key, $alg)){
                return;
            }
            if ($verifyUrl && !$this->verifyTokenUrl($accessToken, $verifyUrl)){
                return;
            }
        }else{
            // die?
        }

        // get username and email from proxy headers
        $user = isset($_SERVER['HTTP_X_FORWARDED_USER']) ? $_SERVER['HTTP_X_FORWARDED_USER'] : (isset($_SERVER['HTTP_X_AUTHENTIK_UID']) ? $_SERVER['HTTP_X_AUTHENTIK_UID'] : null);
        $userName = isset($_SERVER['HTTP_X_FORWARDED_PREFERRED_USERNAME']) ? $_SERVER['HTTP_X_FORWARDED_PREFERRED_USERNAME'] : (isset($_SERVER['HTTP_X_AUTHENTIK_USERNAME']) ? $_SERVER['HTTP_X_AUTHENTIK_USERNAME'] : null);
        $emailAddress = isset($_SERVER['HTTP_X_FORWARDED_EMAIL']) ? $_SERVER['HTTP_X_FORWARDED_EMAIL'] : (isset($_SERVER['HTTP_X_AUTHENTIK_EMAIL']) ? $_SERVER['HTTP_X_AUTHENTIK_EMAIL'] : null);

        if ($user || $userName || $emailAddress) {
            // attempt SSO login using username and email
            if ($this->sessionH->attemptLoginSSO($user, $userName, $emailAddress)) {
                // logged in successfully
                $this->say("Successful Login!");
                header("Location: /");
                // $this->sessionH->reloadPage();
            } else {
                // unsuccessful login
                $this->say("Sorry, SSO login was incorrect or user not found.");
            }
        } else {
            // missing headers, show error or fallback
            $this->say("SSO authentication headers missing.");
        }

    }

}
?>
