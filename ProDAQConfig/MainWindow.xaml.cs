using System;
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
        private readonly ObservableCollection<string> _availablePorts = new ObservableCollection<string>();
        private readonly DispatcherTimer _telemetryTimer;
        private readonly object _serialLock = new object();
        private SerialPort _serialPort;

        private string _selectedPort;
        private string _statusMessage = "Seleccione un puerto y presione Conectar";
        private string _forceReading = "--";
        private string _encoderReading = "--";
        private string _alarmStatus = "--";
        private double _offsetValue;
        private bool _isConnected;

        public event PropertyChangedEventHandler PropertyChanged;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;

            _telemetryTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(1.5)
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

        private void ConnectButton_Click(object sender, RoutedEventArgs e)
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
                    NewLine = "\r\n"
                };
                _serialPort.Open();
                IsConnected = true;
                StatusMessage = $"Conectado a {SelectedPort}";
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
                ForceReading = await QueryDeviceAsync("R1");
                EncoderReading = await QueryDeviceAsync("R2");
                AlarmStatus = await QueryDeviceAsync("R3");
                StatusMessage = "Lecturas actualizadas";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error leyendo datos: {ex.Message}";
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
                var command = $"WO {formattedValue}"; // Comando de ejemplo para ajustar offset
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
    }
}
