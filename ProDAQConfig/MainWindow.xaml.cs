using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO.Ports;
using System.Linq;
using System.Windows;

namespace ProDAQConfig
{
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        private readonly SerialPort _serialPort = new SerialPort();
        private string _selectedPort = string.Empty;
        private string _statusMessage = string.Empty;
        private string _forceReading = "-";
        private string _encoderReading = "-";
        private string _alarmStatus = "-";
        private double _offsetValue;
        private double _encoderGain;

        public event PropertyChangedEventHandler PropertyChanged;

        public ObservableCollection<string> AvailablePorts { get; } = new ObservableCollection<string>();

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
                    _offsetValue = value;
                    OnPropertyChanged(nameof(OffsetValue));
                }
            }
        }

        public double EncoderGain
        {
            get => _encoderGain;
            set
            {
                if (Math.Abs(_encoderGain - value) > double.Epsilon)
                {
                    _encoderGain = value;
                    OnPropertyChanged(nameof(EncoderGain));
                }
            }
        }

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;
            RefreshAvailablePorts();
            StatusMessage = "Seleccione un puerto para conectar.";
        }

        private void RefreshAvailablePorts()
        {
            AvailablePorts.Clear();
            foreach (var port in SerialPort.GetPortNames().OrderBy(p => p))
            {
                AvailablePorts.Add(port);
            }

            StatusMessage = AvailablePorts.Count == 0
                ? "No se encontraron puertos serie disponibles."
                : "Puertos actualizados.";
        }

        private void RefreshPortsButton_Click(object sender, RoutedEventArgs e)
        {
            RefreshAvailablePorts();
        }

        private void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(SelectedPort))
            {
                StatusMessage = "Seleccione un puerto antes de conectar.";
                return;
            }

            try
            {
                if (_serialPort.IsOpen)
                {
                    _serialPort.Close();
                }

                _serialPort.PortName = SelectedPort;
                _serialPort.BaudRate = 115200;
                _serialPort.Open();
                StatusMessage = $"Conectado a {SelectedPort}.";
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error al conectar: {ex.Message}";
            }
        }

        private void DisconnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (_serialPort.IsOpen)
            {
                _serialPort.Close();
                StatusMessage = "Desconectado.";
            }
            else
            {
                StatusMessage = "No hay conexión activa.";
            }
        }

        private void ReadTelemetryButton_Click(object sender, RoutedEventArgs e)
        {
            if (!EnsureConnected("leer la telemetría"))
            {
                return;
            }

            // Sustituir por lectura real del firmware cuando esté disponible.
            ForceReading = $"{OffsetValue:F3} V";
            EncoderReading = EncoderGain > 0
                ? $"Ganancia: {EncoderGain:F3} pasos/mm"
                : "Sin configurar";
            AlarmStatus = "Lectura simulada";
            StatusMessage = "Telemetría actualizada (simulada).";
        }

        private void ApplyOffsetButton_Click(object sender, RoutedEventArgs e)
        {
            if (!EnsureConnected("enviar el offset"))
            {
                return;
            }

            StatusMessage = $"Offset enviado: {OffsetValue:F3} V.";
        }

        private void ApplyEncoderGainButton_Click(object sender, RoutedEventArgs e)
        {
            if (EncoderGain <= 0)
            {
                StatusMessage = "Introduzca una ganancia del encoder mayor que cero.";
                return;
            }

            if (!EnsureConnected("aplicar la ganancia del encoder"))
            {
                return;
            }

            StatusMessage = $"Ganancia del encoder aplicada: {EncoderGain:F3} pasos/mm.";
        }

        private bool EnsureConnected(string actionDescription)
        {
            if (_serialPort.IsOpen)
            {
                return true;
            }

            StatusMessage = $"Debe conectarse a un puerto serie antes de {actionDescription}.";
            return false;
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
