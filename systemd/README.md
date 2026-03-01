# Systemd units

* `bme280.timer` to kick off the process
* `bme280.service` to collect and read the sensor"

```bash
# Install as a user unit (recommended):
mkdir -p ~/.config/systemd/user
cp systemd/bme280.service systemd/bme280.timer ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now bme280.timer
systemctl --user status bme280.timer

# If this should run even when you are not logged in, enable linger:
# sudo loginctl enable-linger $(whoami)
```
