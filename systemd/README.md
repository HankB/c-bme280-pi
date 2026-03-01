

sudo cp systemd/bme280.service /etc/systemd/system/
sudo cp systemd/bme280.timer /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now bme280.timer
sudo systemctl status bme280.timer