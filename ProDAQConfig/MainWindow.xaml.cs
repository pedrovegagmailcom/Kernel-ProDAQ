using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;

namespace ProDAQConfig
{
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
        private DateTime _lastTelemetryTimestamp = DateTime.MinValue;
        private readonly Queue<DateTime> _telemetryTimestamps = new Queue<DateTime>();

        private string _selectedPort;
        private string _statusMessage = "Seleccione un puerto y presione Conectar";
        private string _communicationStatus = "Sin comunicación";
        private string _forceReading = "--";
        private string _voltageReading = "--";
        private string _encoderReading = "--";
        private string _alarmStatus = "--";
        private string _dataRateStatus = "--";
        private double _coarseOffsetValue;
        private double _fineOffsetAdjustment;
        private bool _suppressFineOffsetAutoApply;
        private double _encoderGain = 1.0;
        private double _speedSetpoint = 100.0;
        private bool _isEncoderInverted;
        private bool _isCompresometerMode;
        private bool _isConnected;
        private bool _communicationHealthy;
        private bool _isManualUpActive;
        private bool _isManualDownActive;
        private bool _isManualStopActive = true;

        // Estado de alarmas individuales
        private bool _alarmMotorActive;
        private bool _alarmCompresorActive;
        private bool _alarmFciActive;
        private bool _alarmTraccionActive;
        private bool _alarmFcsActive;
        private bool _alarmSetaActive;
        private bool _alarmCeroActive;
        private bool _alarmCelulaActive;

        // Resumen
        private int _activeAlarmCount;
        private bool _hasActiveAlarms;

        // Bits de estado
        private bool _statusUpDownOn;
        private bool _statusStopOn;
        private bool _statusRemoteOn;

        private enum ManualControlState
        {
            Up,
            Down,
            Stop
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;

            Loaded += MainWindow_OnLoaded;

            _telemetryTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(0.01)
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

        public string CommunicationStatus
        {
            get => _communicationStatus;
            private set
            {
                if (_communicationStatus != value)
                {
                    _communicationStatus = value;
                    OnPropertyChanged(nameof(CommunicationStatus));
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

        public string VoltageReading
        {
            get => _voltageReading;
            set
            {
                if (_voltageReading != value)
                {
                    _voltageReading = value;
                    OnPropertyChanged(nameof(VoltageReading));
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

        public string DataRateStatus
        {
            get => _dataRateStatus;
            private set
            {
                if (_dataRateStatus != value)
                {
                    _dataRateStatus = value;
                    OnPropertyChanged(nameof(DataRateStatus));
                }
            }
        }

        public double CoarseOffsetValue
        {
            get => _coarseOffsetValue;
            set
            {
                var clamped = Math.Max(-1.0, Math.Min(1.0, value));
                var rounded = Math.Round(clamped, 3);
                if (Math.Abs(_coarseOffsetValue - rounded) > double.Epsilon)
                {
                    _coarseOffsetValue = rounded;
                    OnPropertyChanged(nameof(CoarseOffsetValue));
                    OnPropertyChanged(nameof(OffsetValue));
                }
            }
        }

        public double FineOffsetAdjustment
        {
            get => _fineOffsetAdjustment;
            set
            {
                var clamped = Math.Max(-0.1, Math.Min(0.1, value));
                var rounded = Math.Round(clamped, 3);
                if (Math.Abs(_fineOffsetAdjustment - rounded) > double.Epsilon)
                {
                    _fineOffsetAdjustment = rounded;
                    OnPropertyChanged(nameof(FineOffsetAdjustment));
                    OnPropertyChanged(nameof(OffsetValue));

                    if (!_suppressFineOffsetAutoApply && _serialPort != null && IsConnected)
                    {
                        _ = ApplyOffsetAsync();
                    }
                }
            }
        }

        public double OffsetValue => Math.Round(Math.Max(-1.0, Math.Min(1.0, _coarseOffsetValue + _fineOffsetAdjustment)), 3);

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

        public bool CommunicationHealthy
        {
            get => _communicationHealthy;
            private set
            {
                if (_communicationHealthy != value)
                {
                    _communicationHealthy = value;
                    OnPropertyChanged(nameof(CommunicationHealthy));
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

        public bool IsCompresometerMode
        {
            get => _isCompresometerMode;
            set
            {
                if (_isCompresometerMode != value)
                {
                    _isCompresometerMode = value;
                    OnPropertyChanged(nameof(IsCompresometerMode));
                }
            }
        }

        #region Propiedades de alarmas

        public bool AlarmMotorActive
        {
            get => _alarmMotorActive;
            private set
            {
                if (_alarmMotorActive != value)
                {
                    _alarmMotorActive = value;
                    OnPropertyChanged(nameof(AlarmMotorActive));
                }
            }
        }

        public bool AlarmCompresorActive
        {
            get => _alarmCompresorActive;
            private set
            {
                if (_alarmCompresorActive != value)
                {
                    _alarmCompresorActive = value;
                    OnPropertyChanged(nameof(AlarmCompresorActive));
                }
            }
        }

        public bool AlarmFCIActive
        {
            get => _alarmFciActive;
            private set
            {
                if (_alarmFciActive != value)
                {
                    _alarmFciActive = value;
                    OnPropertyChanged(nameof(AlarmFCIActive));
                }
            }
        }

        public bool AlarmTraccionActive
        {
            get => _alarmTraccionActive;
            private set
            {
                if (_alarmTraccionActive != value)
                {
                    _alarmTraccionActive = value;
                    OnPropertyChanged(nameof(AlarmTraccionActive));
                }
            }
        }

        public bool AlarmFCSActive
        {
            get => _alarmFcsActive;
            private set
            {
                if (_alarmFcsActive != value)
                {
                    _alarmFcsActive = value;
                    OnPropertyChanged(nameof(AlarmFCSActive));
                }
            }
        }

        public bool AlarmSetaActive
        {
            get => _alarmSetaActive;
            private set
            {
                if (_alarmSetaActive != value)
                {
                    _alarmSetaActive = value;
                    OnPropertyChanged(nameof(AlarmSetaActive));
                }
            }
        }

        public bool AlarmCeroActive
        {
            get => _alarmCeroActive;
            private set
            {
                if (_alarmCeroActive != value)
                {
                    _alarmCeroActive = value;
                    OnPropertyChanged(nameof(AlarmCeroActive));
                }
            }
        }

        public bool AlarmCelulaActive
        {
            get => _alarmCelulaActive;
            private set
            {
                if (_alarmCelulaActive != value)
                {
                    _alarmCelulaActive = value;
                    OnPropertyChanged(nameof(AlarmCelulaActive));
                }
            }
        }

        public int ActiveAlarmCount
        {
            get => _activeAlarmCount;
            private set
            {
                if (_activeAlarmCount != value)
                {
                    _activeAlarmCount = value;
                    OnPropertyChanged(nameof(ActiveAlarmCount));
                }
            }
        }

        public bool HasActiveAlarms
        {
            get => _hasActiveAlarms;
            private set
            {
                if (_hasActiveAlarms != value)
                {
                    _hasActiveAlarms = value;
                    OnPropertyChanged(nameof(HasActiveAlarms));
                }
            }
        }

        public bool StatusUpDownOn
        {
            get => _statusUpDownOn;
            private set
            {
                if (_statusUpDownOn != value)
                {
                    _statusUpDownOn = value;
                    OnPropertyChanged(nameof(StatusUpDownOn));
                }
            }
        }

        public bool StatusStopOn
        {
            get => _statusStopOn;
            private set
            {
                if (_statusStopOn != value)
                {
                    _statusStopOn = value;
                    OnPropertyChanged(nameof(StatusStopOn));
                }
            }
        }

        public bool StatusRemoteOn
        {
            get => _statusRemoteOn;
            private set
            {
                if (_statusRemoteOn != value)
                {
                    _statusRemoteOn = value;
                    OnPropertyChanged(nameof(StatusRemoteOn));
                }
            }
        }

        public bool IsManualUpActive
        {
            get => _isManualUpActive;
            private set
            {
                if (_isManualUpActive != value)
                {
                    _isManualUpActive = value;
                    OnPropertyChanged(nameof(IsManualUpActive));
                }
            }
        }

        public bool IsManualDownActive
        {
            get => _isManualDownActive;
            private set
            {
                if (_isManualDownActive != value)
                {
                    _isManualDownActive = value;
                    OnPropertyChanged(nameof(IsManualDownActive));
                }
            }
        }

        public bool IsManualStopActive
        {
            get => _isManualStopActive;
            private set
            {
                if (_isManualStopActive != value)
                {
                    _isManualStopActive = value;
                    OnPropertyChanged(nameof(IsManualStopActive));
                }
            }
        }

        #endregion

        private async void TelemetryTimerOnTick(object sender, EventArgs e)
        {
            await RequestTelemetryAsync();
        }

        private async void MainWindow_OnPreviewKeyDown(object sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.Up:
                    await MoveUpAsync();
                    e.Handled = true;
                    break;
                case Key.Down:
                    await MoveDownAsync();
                    e.Handled = true;
                    break;
                case Key.Left:
                case Key.Right:
                case Key.Space:
                    await StopAsync();
                    e.Handled = true;
                    break;
            }
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

        private void UpdateCommunicationHealth(bool telemetryReceived)
        {
            if (telemetryReceived)
            {
                _lastTelemetryTimestamp = DateTime.UtcNow;
            }

            var now = DateTime.UtcNow;
            var isHealthy = IsConnected && (now - _lastTelemetryTimestamp) < TimeSpan.FromSeconds(2);

            CommunicationHealthy = isHealthy;
            CommunicationStatus = isHealthy
                ? "Comunicación con la máquina activa"
                : "Sin comunicación con la máquina";

            if (!isHealthy)
            {
                DataRateStatus = "--";
                _telemetryTimestamps.Clear();
            }
        }

        private void UpdateDataRate()
        {
            if (!IsConnected)
            {
                DataRateStatus = "--";
                _telemetryTimestamps.Clear();
                return;
            }

            var now = DateTime.UtcNow;
            _telemetryTimestamps.Enqueue(now);

            while (_telemetryTimestamps.Count > 0 && (now - _telemetryTimestamps.Peek()) > TimeSpan.FromSeconds(5))
            {
                _telemetryTimestamps.Dequeue();
            }

            if (_telemetryTimestamps.Count < 2)
            {
                DataRateStatus = "--";
                return;
            }

            var windowStart = _telemetryTimestamps.Peek();
            var elapsedSeconds = (now - windowStart).TotalSeconds;
            if (elapsedSeconds <= 0)
            {
                DataRateStatus = "--";
                return;
            }

            var samples = _telemetryTimestamps.Count - 1; // intervals between samples
            var rate = samples / elapsedSeconds;
            DataRateStatus = $"{rate:F1} Hz";
        }

        private async void MainWindow_OnLoaded(object sender, RoutedEventArgs e)
        {
            await AttemptAutoConnectionAsync();
        }

        private async void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (IsConnected)
            {
                StatusMessage = "Ya existe una conexión activa";
                return;
            }

            await ConnectToPortAsync(SelectedPort, false);
        }

        private async Task AttemptAutoConnectionAsync()
        {
            var portsToTry = _availablePorts.ToList();
            if (portsToTry.Count == 0)
            {
                StatusMessage = "No se detectan puertos disponibles";
                return;
            }

            var cancellation = new CancellationTokenSource();
            var autoConnectDialog = new AutoConnectDialog(cancellation)
            {
                Owner = this
            };

            autoConnectDialog.Show();

            try
            {
                foreach (var port in portsToTry)
                {
                    if (cancellation.IsCancellationRequested)
                    {
                        StatusMessage = "Búsqueda cancelada";
                        return;
                    }

                    autoConnectDialog.ProgressMessage = $"Buscando electrónica en {port}...";
                    StatusMessage = autoConnectDialog.ProgressMessage;

                    var connected = await ConnectToPortAsync(port, true);
                    if (connected)
                    {
                        autoConnectDialog.ProgressMessage = $"Conexión automática establecida en {port}";
                        StatusMessage = autoConnectDialog.ProgressMessage;
                        return;
                    }
                }

                StatusMessage = "No se pudo conectar automáticamente a ningún puerto";
                autoConnectDialog.ProgressMessage = StatusMessage;
            }
            finally
            {
                autoConnectDialog.Close();
            }
        }

        private async Task<bool> ConnectToPortAsync(string portName, bool isAuto)
        {
            if (string.IsNullOrWhiteSpace(portName))
            {
                if (!isAuto)
                {
                    StatusMessage = "Seleccione un puerto";
                }

                return false;
            }

            SerialPort candidatePort = null;

            try
            {
                candidatePort = new SerialPort(portName, 115200, Parity.None, 8, StopBits.One)
                {
                    Encoding = Encoding.ASCII,
                    ReadTimeout = 1000,
                    WriteTimeout = 1000,
                    NewLine = "\r"
                };
                candidatePort.Open();
                candidatePort.DtrEnable = true;
                candidatePort.WriteLine("RI");
                var response = candidatePort.ReadLine()?.Trim();

                if (!string.Equals(response, "RABBIT", StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException($"Respuesta inesperada de RI: {response ?? "<vacía>"}");
                }

                _serialPort = candidatePort;
                SelectedPort = portName;
                IsConnected = true;
                _telemetryTimestamps.Clear();

                await LoadDeviceConfigurationAsync();
                StatusMessage = $"Conectado a {portName} [{response}]";
                UpdateCommunicationHealth(false);
                _telemetryTimer.Start();

                return true;
            }
            catch (Exception ex)
            {
                if (_serialPort == null)
                {
                    candidatePort?.Dispose();
                    Disconnect();
                }
                else
                {
                    Disconnect();
                }

                if (!isAuto)
                {
                    StatusMessage = $"Error al conectar: {ex.Message}";
                }

                return false;
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
            _lastTelemetryTimestamp = DateTime.MinValue;
            CommunicationStatus = "Desconectado";
            CommunicationHealthy = false;
            DataRateStatus = "--";
            _telemetryTimestamps.Clear();
            SetManualControlState(ManualControlState.Stop);
        }

        private async void ReadTelemetryButton_Click(object sender, RoutedEventArgs e)
        {
            await RequestTelemetryAsync();
        }

        private async Task RequestTelemetryAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                UpdateCommunicationHealth(false);
                return;
            }

            var telemetrySucceeded = false;

            try
            {
                var forceResponse = await QueryDeviceAsync("R1");
                UpdateForceReading(forceResponse);

                var voltageResponse = await QueryDeviceAsync("R3");
                UpdateVoltageReading(voltageResponse);

                var encoderResponse = await QueryDeviceAsync("R2");
                ParseEncoderReading(encoderResponse);

                var (alarmByte, statusByte) = await QueryAlarmBytesAsync();
                AlarmStatus = FormatAlarmStatus(alarmByte, statusByte);

                telemetrySucceeded = true;
                StatusMessage = "Lecturas actualizadas";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error leyendo datos: {ex.Message}";
            }

            UpdateCommunicationHealth(telemetrySucceeded);

            if (telemetrySucceeded)
            {
                UpdateDataRate();
            }
        }

        private async Task LoadDeviceConfigurationAsync()
        {
            try
            {
                var offsetResponse = await QueryDeviceAsync("RP02");
                if (double.TryParse(offsetResponse, NumberStyles.Float, CultureInfo.InvariantCulture, out var offset))
                {
                    ApplyOffsetFromDevice(offset);
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

                var modeResponse = await QueryDeviceAsync("RP05");
                if (int.TryParse(modeResponse, NumberStyles.Integer, CultureInfo.InvariantCulture, out var mode))
                {
                    IsCompresometerMode = mode == 1;
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"No se pudo leer la configuración: {ex.Message}";
            }
        }

        private void ApplyOffsetFromDevice(double offset)
        {
            _suppressFineOffsetAutoApply = true;
            try
            {
                var coarse = Math.Max(-0.2, Math.Min(0.2, offset));
                CoarseOffsetValue = Math.Round(coarse, 3);

                var remainder = offset - coarse;
                FineOffsetAdjustment = Math.Round(Math.Max(-0.0001, Math.Min(0.0001, remainder)), 4);
            }
            finally
            {
                _suppressFineOffsetAutoApply = false;
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

            AlarmMotorActive = (alarmByte & (1 << 0)) != 0;
            AlarmCompresorActive = (alarmByte & (1 << 1)) != 0;
            AlarmFCIActive = (alarmByte & (1 << 2)) != 0;
            AlarmTraccionActive = (alarmByte & (1 << 3)) != 0;
            AlarmFCSActive = (alarmByte & (1 << 4)) != 0;
            AlarmSetaActive = (alarmByte & (1 << 5)) != 0;
            AlarmCeroActive = (alarmByte & (1 << 6)) != 0;
            AlarmCelulaActive = (alarmByte & (1 << 7)) != 0;

            ActiveAlarmCount = activeAlarms.Count;
            HasActiveAlarms = ActiveAlarmCount > 0;

            var alarmText = activeAlarms.Count > 0
                ? $"Alarmas: {string.Join(", ", activeAlarms)}"
                : "Sin alarmas";

            StatusUpDownOn = (statusByte & (1 << 0)) != 0;
            StatusStopOn = (statusByte & (1 << 1)) != 0;
            StatusRemoteOn = (statusByte & (1 << 2)) != 0;

            var statusDescriptions = new[]
            {
                ("Up/Down", 0),
                ("Stop", 1),
                ("Remoto", 2)
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
                        _serialPort.ReadLine();
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
                // AHORA SIN "mm", la unidad la añade el XAML
                EncoderReading = $"{adjusted:F3}";
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

        private void UpdateVoltageReading(string voltageResponse)
        {
            if (TryParseDouble(voltageResponse, out var voltage))
            {
                VoltageReading = $"{voltage:F4}";
            }
            else
            {
                VoltageReading = voltageResponse;
            }
        }

        private async void ZeroForceButton_Click(object sender, RoutedEventArgs e)
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de poner a cero la fuerza";
                return;
            }

            if (!_lastForceValue.HasValue)
            {
                StatusMessage = "No hay lectura de fuerza válida para poner a cero";
                return;
            }

            try
            {
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine("WZ");
                        _serialPort.ReadLine();
                    }
                });

                _forceZeroReference = null;
                ForceReading = $"{0.0:F3}";
                StatusMessage = "Cero de fuerza solicitado al kernel";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al solicitar cero de fuerza: {ex.Message}";
            }
        }

        private void SetManualControlState(ManualControlState state)
        {
            IsManualUpActive = state == ManualControlState.Up;
            IsManualDownActive = state == ManualControlState.Down;
            IsManualStopActive = state == ManualControlState.Stop;
        }

        private void ZeroEncoderButton_Click(object sender, RoutedEventArgs e)
        {
            if (_lastEncoderValueMm.HasValue)
            {
                _encoderZeroReference = _lastEncoderValueMm.Value;
                EncoderReading = $"{0.0:F3}";
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

        private async void ApplyMachineModeButton_OnClick(object sender, RoutedEventArgs e)
        {
            await ApplyMachineModeAsync();
        }

        private void ManageCellConfigButton_OnClick(object sender, RoutedEventArgs e)
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de gestionar CellConfig.";
                return;
            }

            var window = new CellConfigWindow(QueryDeviceAsync)
            {
                Owner = this
            };

            window.ShowDialog();
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

        private async Task ApplyMachineModeAsync()
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de aplicar el modo de compresómetro";
                return;
            }

            try
            {
                var command = $"WP05{(IsCompresometerMode ? 1 : 0)}";
                await Task.Run(() =>
                {
                    lock (_serialLock)
                    {
                        _serialPort.WriteLine(command);
                        _serialPort.ReadLine();
                    }
                });

                StatusMessage = IsCompresometerMode
                    ? "Modo compresómetro aplicado"
                    : "Modo dinamómetro aplicado";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error aplicando modo de compresómetro: {ex.Message}";
            }
        }

        private async void ApplySpeedSetpointButton_Click(object sender, RoutedEventArgs e)
        {
            await ApplySpeedSetpointAsync();
        }

        private async Task<bool> ApplySpeedSetpointAsync(bool updateStatusMessage = true)
        {
            if (_serialPort == null || !IsConnected)
            {
                StatusMessage = "Debe conectarse a un puerto antes de enviar la consigna de velocidad";
                return false;
            }

            var targetSpeed = SpeedSetpoint;

            if (targetSpeed < 0)
            {
                StatusMessage = "La velocidad no puede ser negativa";
                return false;
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

                if (updateStatusMessage)
                {
                    StatusMessage = $"Consigna de velocidad enviada: {targetSpeed:F1} mm/min";
                }
                return true;
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al enviar la consigna: {ex.Message}";
                return false;
            }
        }

        private async void MoveUpButton_Click(object sender, RoutedEventArgs e)
        {
            await MoveUpAsync();
        }

        private async void MoveDownButton_Click(object sender, RoutedEventArgs e)
        {
            await MoveDownAsync();
        }

        private async void StopButton_Click(object sender, RoutedEventArgs e)
        {
            await StopAsync();
        }

        private async Task MoveUpAsync()
        {
            if (!await ApplySpeedSetpointAsync(false))
            {
                return;
            }

            await SendMachineCommandAsync("WF", "Comando SUBIR enviado", "enviar comando SUBIR", ManualControlState.Up);
        }

        private async Task MoveDownAsync()
        {
            if (!await ApplySpeedSetpointAsync(false))
            {
                return;
            }

            await SendMachineCommandAsync("WR", "Comando BAJAR enviado", "enviar comando BAJAR", ManualControlState.Down);
        }

        private async Task StopAsync()
        {
            await SendMachineCommandAsync("WS", "Comando PARAR enviado", "enviar comando PARAR", ManualControlState.Stop);
        }

        private async Task SendMachineCommandAsync(string command, string successMessage, string errorAction, ManualControlState? manualState = null)
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
                if (manualState.HasValue)
                {
                    SetManualControlState(manualState.Value);
                }
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al {errorAction}: {ex.Message}";
            }
        }
    }
}
