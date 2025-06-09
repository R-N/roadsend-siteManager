<?php

require_once 'JWT.php';
use \Firebase\JWT\JWT;
class SM_ssoLogin extends SM_module {

    function moduleConfig() {
        // no user input, no forms
    }

    function canModuleLoad() {
        return $this->sessionH->directive['useMemberSystem'];
    }

    function moduleThink() {
        // If already logged in, just redirect
        if (!$this->sessionH->isGuest()) {
            header("Location: /");
            return;
        }

        $accessToken = '';
        if (isset($_SERVER['HTTP_X_FORWARDED_ACCESS_TOKEN'])){
            $accessToken = $_SERVER['HTTP_X_FORWARDED_ACCESS_TOKEN'];

            global $SM_siteManager;
            $sName = $SM_siteManager->findSMfile('public_key', 'keys', 'pem', true);
            $publicKey = file_get_contents($sName);
            $pubKeyResource = openssl_pkey_get_public($publicKey);

            if ($pubKeyResource === false) {
                $this->say("Failed to load public key");
                return;
            }

            try {
                $decoded = JWT::decode($accessToken, $publicKey, array('RS256'));
            } catch (Exception $e) {
                $this->say("Invalid token: " . $e->getMessage());
                return;
            }
        }else if (isset($_SERVER['HTTP_X_AUTHENTIK_JWT'])){
            $accessToken = $_SERVER['HTTP_X_AUTHENTIK_JWT'];
            // TODO: we just blindly trust authentik lol
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
