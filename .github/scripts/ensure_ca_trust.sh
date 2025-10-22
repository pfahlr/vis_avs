#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"

if [[ -z "${DISTRO}" ]]; then
  echo "DISTRO environment variable must be provided" >&2
  exit 1
fi

if [[ "${DISTRO}" != "ubuntu" ]]; then
  exit 0
fi

apt-get update
apt-get install -y ca-certificates
git config --global http.sslcainfo /etc/ssl/certs/ca-certificates.crt
unset CURL_CA_BUNDLE SSL_CERT_FILE
