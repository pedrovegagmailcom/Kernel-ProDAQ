using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;

namespace ProDAQConfig
{
    /// <summary>
    /// Lógica de interacción para MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        private const double MaxSpeedSetpoint = 500.0;

        private readonly ObservableCollection<string> _availablePorts = new ObservableCollection<string>();
        private readonly DispatcherTimer _telemetryTimer;
        private readonly object _serialLock = new object();
        private SerialPort _serialPort;

        private double? _forceZeroReference;
        private double? _encoderZeroReference;
        private double? _lastForceValue;
        private double? _lastEncoderValueMm;

        private string _selectedPort;
        private string _statusMessage = "Seleccione un puerto y presione Conectar";
        private string _forceReading = "--";
        private string _encoderReading = "--";
        private string _alarmStatus = "--";
        private double _offsetValue;
        private double _encoderGain = 1.0;
        private double _speedSetpoint = 100.0;
        private bool _isEncoderInverted;
        private bool _isConnected;

        public event PropertyChangedEventHandler PropertyChanged;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;

            _telemetryTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(0.1)
            };
            _telemetryTimer.Tick += TelemetryTimerOnTick;

            RefreshPorts();
        }

        public ObservableCollection<string> AvailablePorts => _availablePorts;

        public string SelectedPort
        {
            get => _selectedPort;
            set
            {
                if (_selectedPort != value)
                {
                    _selectedPort = value;
                    OnPropertyChanged(nameof(SelectedPort));
                }
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set
            {
                if (_statusMessage != value)
                {
                    _statusMessage = value;
                    OnPropertyChanged(nameof(StatusMessage));
                }
            }
        }

        public string ForceReading
        {
            get => _forceReading;
            set
            {
                if (_forceReading != value)
                {
                    _forceReading = value;
                    OnPropertyChanged(nameof(ForceReading));
                }
            }
        }

        public string EncoderReading
        {
            get => _encoderReading;
            set
            {
                if (_encoderReading != value)
                {
                    _encoderReading = value;
                    OnPropertyChanged(nameof(EncoderReading));
                }
            }
        }

        public string AlarmStatus
        {
            get => _alarmStatus;
            set
            {
                if (_alarmStatus != value)
                {
                    _alarmStatus = value;
                    OnPropertyChanged(nameof(AlarmStatus));
                }
            }
        }

        public double OffsetValue
        {
            get => _offsetValue;
            set
            {
                if (Math.Abs(_offsetValue - value) > double.Epsilon)
                {
                    _offsetValue = Math.Round(value, 3);
                    OnPropertyChanged(nameof(OffsetValue));
                }
            }
        }

        public bool IsConnected
        {
            get => _isConnected;
            private set
            {
                if (_isConnected != value)
                {
                    _isConnected = value;
                    OnPropertyChanged(nameof(IsConnected));
                }
            }
        }

        public double EncoderGain
        {
            get => _encoderGain;
            set
            {
                var rounded = Math.Round(value, 4);
                if (Math.Abs(_encoderGain - rounded) > double.Epsilon)
                {
                    _encoderGain = rounded;
                    OnPropertyChanged(nameof(EncoderGain));
                }
            }
        }

        public double SpeedSetpoint
        {
            get => _speedSetpoint;
            set
            {
                var clamped = Math.Max(0.0, Math.Min(value, MaxSpeedSetpoint));
                var rounded = Math.Round(clamped, 1);
                if (Math.Abs(_speedSetpoint - rounded) > double.Epsilon)
                {
                    _speedSetpoint = rounded;
                    OnPropertyChanged(nameof(SpeedSetpoint));
                }
            }
        }

        public bool IsEncoderInverted
        {
            get => _isEncoderInverted;
            set
            {
                if (_isEncoderInverted != value)
                {
                    _isEncoderInverted = value;
                    OnPropertyChanged(nameof(IsEncoderInverted));
                }
            }
        }

        private async void TelemetryTimerOnTick(object sender, EventArgs e)
        {
            await RequestTelemetryAsync();
        }

        private void RefreshPorts()
        {
            var ports = SerialPort.GetPortNames()
                .OrderBy(p => p)
                .ToList();

            _availablePorts.Clear();
            foreach (var port in ports)
            {
                _availablePorts.Add(port);
            }

            if (!ports.Contains(SelectedPort))
            {
                SelectedPort = ports.FirstOrDefault();
            }

            StatusMessage = ports.Count == 0
                ? "No se detectan puertos disponibles"
                : "Seleccione el puerto que desea utilizar";
        }

        private async void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (IsConnected)
            {
                StatusMessage = "Ya existe una conexión activa";
                return;
            }

            if (string.IsNullOrWhiteSpace(SelectedPort))
            {
                StatusMessage = "Seleccione un puerto";
                return;
            }

            try
            {
                _serialPort = new SerialPort(SelectedPort, 115200, Parity.None, 8, StopBits.One)
                {
                    Encoding = Encoding.ASCII,
                    ReadTimeout = 1000,
                    WriteTimeout = 1000,
                    NewLine = "\r"
                };
                _serialPort.Open();
                IsConnected = true;
                _serialPort.DtrEnable = true;
                _serialPort.WriteLine("RI");
                var response = _serialPort.ReadLine();
                await LoadDeviceConfigurationAsync();
                StatusMessage = $"Conectado a {SelectedPort} [{response}]";
                _telemetryTimer.Start();
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al conectar: {ex.Message}";
                Disconnect();
            }
        }

        private void DisconnectButton_Click(object sender, RoutedEventArgs e)
        {
            Disconnect();
        }

        private void Disconnect()
        {
            _telemetryTimer.Stop();

            if (_serialPort != null)
            {
                try
                {
                    if (_serialPort.IsOpen)
                    {
                        _serialPort.Close();
                    }
                }
                catch (Exception ex)
                {
                    StatusMessage = $"Error al cerrar el puerto: {ex.Message}";
                }
            }

            _serialPort = null;
            IsConnected = false;
        }

        private async void ReadTelemetryButton_Click(object sender, RoutedEventArgs e)
        {
            await RequestTelemetryAsync();
        }

        private async Task RequestTelemetryAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                return;
            }

            try
            {
                var forceResponse = await QueryDeviceAsync("R1");
                UpdateForceReading(forceResponse);
                var encoderResponse = await QueryDeviceAsync("R2");
                ParseEncoderReading(encoderResponse);
                var (alarmByte, statusByte) = await QueryAlarmBytesAsync();
                AlarmStatus = FormatAlarmStatus(alarmByte, statusByte);
                StatusMessage = "Lecturas actualizadas";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error leyendo datos: {ex.Message}";
            }
        }

        private async Task LoadDeviceConfigurationAsync()
        {
            try
            {
                var offsetResponse = await QueryDeviceAsync("RP02");
                if (double.TryParse(offsetResponse, NumberStyles.Float, CultureInfo.InvariantCulture, out var offset))
                {
                    OffsetValue = offset;
                }

                var gainResponse = await QueryDeviceAsync("RP01");
                if (double.TryParse(gainResponse, NumberStyles.Float, CultureInfo.InvariantCulture, out var gain) && gain > 0)
                {
                    EncoderGain = gain;
                }

                var polarityResponse = await QueryDeviceAsync("RP03");
                if (int.TryParse(polarityResponse, NumberStyles.Integer, CultureInfo.InvariantCulture, out var polarity))
                {
                    IsEncoderInverted = polarity < 0;
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"No se pudo leer la configuración: {ex.Message}";
            }
        }

        private Task<string> QueryDeviceAsync(string command)
        {
            return Task.Run(() =>
            {
                lock (_serialLock)
                {
                    if (_serialPort == null || !_serialPort.IsOpen)
                    {
                        throw new InvalidOperationException("El puerto no está abierto");
                    }

                    _serialPort.DiscardInBuffer();
                    _serialPort.WriteLine(command);
                    var response = _serialPort.ReadLine();
                    return response?.Trim() ?? string.Empty;
                }
            });
        }

        private Task<(byte alarmByte, byte statusByte)> QueryAlarmBytesAsync()
        {
            return Task.Run(() =>
            {
                lock (_serialLock)
                {
                    if (_serialPort == null || !_serialPort.IsOpen)
                    {
                        throw new InvalidOperationException("El puerto no está abierto");
                    }

                    _serialPort.DiscardInBuffer();
                    _serialPort.WriteLine("RS");

                    var buffer = new byte[3];
                    var read = 0;
                    while (read < buffer.Length)
                    {
                        var received = _serialPort.Read(buffer, read, buffer.Length - read);
                        if (received == 0)
                        {
                            throw new TimeoutException("Tiempo de espera agotado leyendo RS");
                        }

                        read += received;
                    }

                    if (buffer[2] != '\r')
                    {
                        throw new InvalidOperationException("Respuesta RS inválida (sin terminador)");
                    }

                    return (buffer[0], buffer[1]);
                }
            });
        }

        private string FormatAlarmStatus(byte alarmByte, byte statusByte)
        {
            var alarmNames = new[]
            {
                "Motor",
                "Compresor",
                "FCI",
                "Tracción",
                "FCS",
                "Seta",
                "Cero",
                "Célula"
            };

            var activeAlarms = new List<string>();
            for (var i = 0; i < alarmNames.Length; i++)
            {
                if ((alarmByte & (1 << i)) != 0)
                {
                    activeAlarms.Add(alarmNames[i]);
                }
            }

            var alarmText = activeAlarms.Count > 0
                ? $"Alarmas: {string.Join(", ", activeAlarms)}"
                : "Sin alarmas";

            var statusDescriptions = new[]
            {
                ($"Up/Down", 0),
                ($"Stop", 1),
                ($"Remoto", 2)
            };

            var statusParts = new List<string>();
            foreach (var (label, bit) in statusDescriptions)
            {
                var isActive = (statusByte & (1 << bit)) != 0;
                statusParts.Add($"{label}: {(isActive ? "ON" : "OFF")}");
            }

            var statusText = $"Estado -> {string.Join(", ", statusParts)}";

            return $"{alarmText} | {statusText}";
        }


        private async void ApplyOffsetButton_Click(object sender, RoutedEventArgs e)
        {
            await ApplyOffsetAsync();
        }

        private async Task ApplyOffsetAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de aplicar el offset";
                return;
            }

            try
            {
                var formattedValue = OffsetValue.ToString("F3", CultureInfo.InvariantCulture);
                var command = $"WP02{formattedValue}";
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine(command);
                        _serialPort.ReadLine(); // se asume eco o confirmación
                    }
                });

                StatusMessage = $"Offset aplicado: {OffsetValue:F3} V";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error aplicando offset: {ex.Message}";
            }
        }

        private void RefreshPortsButton_Click(object sender, RoutedEventArgs e)
        {
            RefreshPorts();
        }

        protected override void OnClosed(EventArgs e)
        {
            base.OnClosed(e);
            Disconnect();
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private void ParseEncoderReading(string encoderResponse)
        {
            if (TryParseDouble(encoderResponse, out var millimeters))
            {
                _lastEncoderValueMm = millimeters;
                var adjusted = millimeters - (_encoderZeroReference ?? 0.0);
                EncoderReading = $"{adjusted:F3} mm";
            }
            else
            {
                _lastEncoderValueMm = null;
                EncoderReading = encoderResponse;
            }
        }

        private async void ApplyEncoderGainButton_Click(object sender, RoutedEventArgs e)
        {
            await ApplyEncoderGainAsync();
        }

        private void UpdateForceReading(string forceResponse)
        {
            if (TryParseDouble(forceResponse, out var force))
            {
                _lastForceValue = force;
                var adjusted = force - (_forceZeroReference ?? 0.0);
                ForceReading = $"{adjusted:F3}";
            }
            else
            {
                _lastForceValue = null;
                ForceReading = forceResponse;
            }
        }

        private void ZeroForceButton_Click(object sender, RoutedEventArgs e)
        {
            if (_lastForceValue.HasValue)
            {
                _forceZeroReference = _lastForceValue.Value;
                ForceReading = $"{0.0:F3}";
                StatusMessage = "Cero de fuerza aplicado";
            }
            else
            {
                StatusMessage = "No hay lectura de fuerza válida para poner a cero";
            }
        }

        private void ZeroEncoderButton_Click(object sender, RoutedEventArgs e)
        {
            if (_lastEncoderValueMm.HasValue)
            {
                _encoderZeroReference = _lastEncoderValueMm.Value;
                EncoderReading = $"{0.0:F3} mm";
                StatusMessage = "Cero de encoder aplicado";
            }
            else
            {
                StatusMessage = "No hay lectura de encoder válida para poner a cero";
            }
        }

        private static bool TryParseDouble(string input, out double value)
        {
            return double.TryParse(input, NumberStyles.Float, CultureInfo.InvariantCulture, out value);
        }

        private async Task ApplyEncoderGainAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de aplicar la ganancia";
                return;
            }

            if (EncoderGain <= 0)
            {
                StatusMessage = "La ganancia debe ser mayor a cero";
                return;
            }

            try
            {
                var formattedValue = EncoderGain.ToString("F4", CultureInfo.InvariantCulture);
                var command = $"WP01{formattedValue}";
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine(command);
                        _serialPort.ReadLine();
                    }
                });

                StatusMessage = $"Ganancia aplicada: {EncoderGain:F4} pasos/mm";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error aplicando ganancia: {ex.Message}";
            }
        }

        private async void ApplyEncoderPolarityButton_OnClick(object sender, RoutedEventArgs e)
        {
            await ApplyEncoderPolarityAsync();
        }

        private async Task ApplyEncoderPolarityAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de aplicar la polaridad";
                return;
            }

            var targetPolarity = IsEncoderInverted ? -1 : 1;

            try
            {
                var command = $"WP03{targetPolarity}";
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine(command);
                        _serialPort.ReadLine();
                    }
                });

                StatusMessage = IsEncoderInverted
                    ? "Polaridad del encoder configurada como invertida"
                    : "Polaridad del encoder configurada como normal";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error aplicando polaridad: {ex.Message}";
            }
        }

        private async void ApplySpeedSetpointButton_Click(object sender, RoutedEventArgs e)
        {
            await ApplySpeedSetpointAsync();
        }

        private async Task ApplySpeedSetpointAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de enviar la consigna de velocidad";
                return;
            }

            var targetSpeed = SpeedSetpoint;

            if (targetSpeed < 0)
            {
                StatusMessage = "La velocidad no puede ser negativa";
                return;
            }

            try
            {
                var formattedValue = targetSpeed.ToString("F1", CultureInfo.InvariantCulture);
                var command = $"WV{formattedValue}";
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine(command);
                        _serialPort.ReadLine();
                    }
                });

                StatusMessage = $"Consigna de velocidad enviada: {targetSpeed:F1} mm/min";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al enviar la consigna: {ex.Message}";
            }
        }

        private async void MoveUpButton_Click(object sender, RoutedEventArgs e)
        {
            await SendMachineCommandAsync("WF", "Comando SUBIR enviado", "enviar comando SUBIR");
        }

        private async void MoveDownButton_Click(object sender, RoutedEventArgs e)
        {
            await SendMachineCommandAsync("WR", "Comando BAJAR enviado", "enviar comando BAJAR");
        }

        private async void StopButton_Click(object sender, RoutedEventArgs e)
        {
            await SendMachineCommandAsync("WS", "Comando PARAR enviado", "enviar comando PARAR");
        }

        private async Task SendMachineCommandAsync(string command, string successMessage, string errorAction)
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de enviar comandos de movimiento";
                return;
            }

            try
            {
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine(command);
                        _serialPort.ReadLine();
                    }
                });

                StatusMessage = successMessage;
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al {errorAction}: {ex.Message}";
            }
        }
    }
}