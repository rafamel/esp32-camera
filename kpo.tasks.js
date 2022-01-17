module.exports = ({ series, exec }) => ({
  build: series(exec('pio', ['run']), exec('pio', ['run', '-t', 'buildfs'])),
  test: exec('pio', ['test']),
  clean: exec('pio', ['run', '-t', 'clean']),
  serial: {
    list: exec('pio', ['device', 'list', '--serial']),
    erase: exec('esptool.py', ['erase_flash']),
    upload: exec('pio', ['run', '-t', 'upload']),
    transfer: exec('pio', ['run', '-t', 'uploadfs']),
    monitor: exec('pio', ['device', 'monitor', '-b', '115200']),
    examine: series(
      exec('esptool.py', ['chip_id']),
      exec('esptool.py', ['flash_id']),
      exec('esptool.py', ['read_mac'])
    )
  }
});
