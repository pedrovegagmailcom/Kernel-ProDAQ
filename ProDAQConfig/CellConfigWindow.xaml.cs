using System;
using System.ComponentModel;
using System.Globalization;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace ProDAQConfig
{
    public partial class CellConfigWindow : Window, INotifyPropertyChanged
    {
        private readonly Func<string, Task<string>> _sendCommandAsync;
        private CellConfigModel _config = new CellConfigModel();
        private bool _isBusy;
        private string _statusMessage = "Listo";

        public CellConfigWindow(Func<string, Task<string>> sendCommandAsync)
        {
            _sendCommandAsync = sendCommandAsync ?? throw new ArgumentNullException(nameof(sendCommandAsync));
            InitializeComponent();
            DataContext = this;
            Loaded += OnLoaded;
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public CellConfigModel Config
        {
            get => _config;
            private set
            {
                if (!Equals(_config, value))
                {
                    _config = value;
                    OnPropertyChanged(nameof(Config));
                }
            }
        }

        public bool IsBusy
        {
            get => _isBusy;
            private set
            {
                if (_isBusy != value)
                {
                    _isBusy = value;
                    OnPropertyChanged(nameof(IsBusy));
                }
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            private set
            {
                if (_statusMessage != value)
                {
                    _statusMessage = value;
                    OnPropertyChanged(nameof(StatusMessage));
                }
            }
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            await LoadFromKernelAsync();
        }

        private async Task LoadFromKernelAsync()
        {
            if (IsBusy)
            {
                return;
            }

            IsBusy = true;
            try
            {
                StatusMessage = "Leyendo configuración de la célula...";
                var response = await _sendCommandAsync("RP04");
                Config = CellConfigModel.FromHex(response);
                StatusMessage = "Configuración cargada desde el kernel.";
            }
            catch (Exception ex)
            {
                StatusMessage = $"No se pudo leer la configuración: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }

        private async Task SaveToKernelAsync()
        {
            if (IsBusy)
            {
                return;
            }

            if (!Config.TryBuildHex(out var hexString, out var validationError))
            {
                StatusMessage = validationError;
                return;
            }

            IsBusy = true;
            try
            {
                StatusMessage = "Enviando configuración al kernel...";
                var command = $"WP04{hexString}";
                var response = await _sendCommandAsync(command);

                if (!string.Equals(response, "OK", StringComparison.OrdinalIgnoreCase))
                {
                    StatusMessage = $"El kernel respondió de forma inesperada: {response}";
                    return;
                }

                StatusMessage = "Configuración guardada correctamente.";
            }
            catch (Exception ex)
            {
                StatusMessage = $"No se pudo guardar la configuración: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }

        private async void LoadButton_OnClick(object sender, RoutedEventArgs e)
        {
            await LoadFromKernelAsync();
        }

        private async void SaveButton_OnClick(object sender, RoutedEventArgs e)
        {
            await SaveToKernelAsync();
        }

        private void CloseButton_OnClick(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    public class CellConfigModel : INotifyPropertyChanged
    {
        private const int CellConfigByteCount = 54;
        private string _serialNumber = string.Empty;
        private int _capacidad;
        private int _limite;
        private float _resolucion;
        private float _x1T;
        private float _x2T;
        private float _x3T;
        private float _x4T;
        private float _x1C;
        private float _x2C;
        private float _x3C;
        private float _x4C;
        private int _overloadT;
        private int _overloadC;

        public event PropertyChangedEventHandler PropertyChanged;

        public string SerialNumber
        {
            get => _serialNumber;
            set
            {
                if (_serialNumber != value)
                {
                    _serialNumber = value ?? string.Empty;
                    OnPropertyChanged(nameof(SerialNumber));
                }
            }
        }

        public int Capacidad
        {
            get => _capacidad;
            set
            {
                if (_capacidad != value)
                {
                    _capacidad = value;
                    OnPropertyChanged(nameof(Capacidad));
                }
            }
        }

        public int Limite
        {
            get => _limite;
            set
            {
                if (_limite != value)
                {
                    _limite = value;
                    OnPropertyChanged(nameof(Limite));
                }
            }
        }

        public float Resolucion
        {
            get => _resolucion;
            set
            {
                if (Math.Abs(_resolucion - value) > double.Epsilon)
                {
                    _resolucion = value;
                    OnPropertyChanged(nameof(Resolucion));
                }
            }
        }

        public float X1T
        {
            get => _x1T;
            set
            {
                if (Math.Abs(_x1T - value) > double.Epsilon)
                {
                    _x1T = value;
                    OnPropertyChanged(nameof(X1T));
                }
            }
        }

        public float X2T
        {
            get => _x2T;
            set
            {
                if (Math.Abs(_x2T - value) > double.Epsilon)
                {
                    _x2T = value;
                    OnPropertyChanged(nameof(X2T));
                }
            }
        }

        public float X3T
        {
            get => _x3T;
            set
            {
                if (Math.Abs(_x3T - value) > double.Epsilon)
                {
                    _x3T = value;
                    OnPropertyChanged(nameof(X3T));
                }
            }
        }

        public float X4T
        {
            get => _x4T;
            set
            {
                if (Math.Abs(_x4T - value) > double.Epsilon)
                {
                    _x4T = value;
                    OnPropertyChanged(nameof(X4T));
                }
            }
        }

        public float X1C
        {
            get => _x1C;
            set
            {
                if (Math.Abs(_x1C - value) > double.Epsilon)
                {
                    _x1C = value;
                    OnPropertyChanged(nameof(X1C));
                }
            }
        }

        public float X2C
        {
            get => _x2C;
            set
            {
                if (Math.Abs(_x2C - value) > double.Epsilon)
                {
                    _x2C = value;
                    OnPropertyChanged(nameof(X2C));
                }
            }
        }

        public float X3C
        {
            get => _x3C;
            set
            {
                if (Math.Abs(_x3C - value) > double.Epsilon)
                {
                    _x3C = value;
                    OnPropertyChanged(nameof(X3C));
                }
            }
        }

        public float X4C
        {
            get => _x4C;
            set
            {
                if (Math.Abs(_x4C - value) > double.Epsilon)
                {
                    _x4C = value;
                    OnPropertyChanged(nameof(X4C));
                }
            }
        }

        public int OverloadT
        {
            get => _overloadT;
            set
            {
                if (_overloadT != value)
                {
                    _overloadT = value;
                    OnPropertyChanged(nameof(OverloadT));
                }
            }
        }

        public int OverloadC
        {
            get => _overloadC;
            set
            {
                if (_overloadC != value)
                {
                    _overloadC = value;
                    OnPropertyChanged(nameof(OverloadC));
                }
            }
        }

        public static CellConfigModel FromHex(string hex)
        {
            if (string.IsNullOrWhiteSpace(hex))
            {
                throw new ArgumentException("La respuesta está vacía.", nameof(hex));
            }

            if (hex.Length != CellConfigByteCount * 2)
            {
                throw new ArgumentException($"Longitud inesperada: se esperaban {CellConfigByteCount * 2} caracteres.", nameof(hex));
            }

            var bytes = new byte[CellConfigByteCount];
            for (var i = 0; i < CellConfigByteCount; i++)
            {
                var high = ParseHexDigit(hex[2 * i]);
                var low = ParseHexDigit(hex[2 * i + 1]);
                bytes[i] = (byte)((high << 4) | low);
            }

            var model = new CellConfigModel();
            var offset = 0;

            model.SerialNumber = Encoding.ASCII.GetString(bytes, offset, 10).TrimEnd('\0', ' ');
            offset += 10;
            model.Capacidad = BitConverter.ToUInt16(bytes, offset);
            offset += 2;
            model.Limite = BitConverter.ToUInt16(bytes, offset);
            offset += 2;
            model.Resolucion = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X1T = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X2T = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X3T = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X4T = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X1C = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X2C = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X3C = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.X4C = BitConverter.ToSingle(bytes, offset);
            offset += 4;
            model.OverloadT = BitConverter.ToUInt16(bytes, offset);
            offset += 2;
            model.OverloadC = BitConverter.ToUInt16(bytes, offset);

            return model;
        }

        public bool TryBuildHex(out string hexString, out string error)
        {
            if (Capacidad < 0 || Capacidad > ushort.MaxValue)
            {
                error = "La capacidad debe estar entre 0 y 65535.";
                hexString = string.Empty;
                return false;
            }

            if (Limite < 0 || Limite > ushort.MaxValue)
            {
                error = "El límite debe estar entre 0 y 65535.";
                hexString = string.Empty;
                return false;
            }

            if (OverloadT < 0 || OverloadT > ushort.MaxValue)
            {
                error = "El overload de tracción debe estar entre 0 y 65535.";
                hexString = string.Empty;
                return false;
            }

            if (OverloadC < 0 || OverloadC > ushort.MaxValue)
            {
                error = "El overload de compresión debe estar entre 0 y 65535.";
                hexString = string.Empty;
                return false;
            }

            var buffer = new byte[CellConfigByteCount];
            var offset = 0;

            var serialBytes = new byte[10];
            var asciiSerial = Encoding.ASCII.GetBytes(SerialNumber ?? string.Empty);
            Array.Copy(asciiSerial, 0, serialBytes, 0, Math.Min(serialBytes.Length, asciiSerial.Length));
            Array.Copy(serialBytes, 0, buffer, offset, serialBytes.Length);
            offset += serialBytes.Length;

            Array.Copy(BitConverter.GetBytes((ushort)Capacidad), 0, buffer, offset, 2);
            offset += 2;
            Array.Copy(BitConverter.GetBytes((ushort)Limite), 0, buffer, offset, 2);
            offset += 2;

            Array.Copy(BitConverter.GetBytes((float)Resolucion), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X1T), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X2T), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X3T), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X4T), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X1C), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X2C), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X3C), 0, buffer, offset, 4);
            offset += 4;
            Array.Copy(BitConverter.GetBytes((float)X4C), 0, buffer, offset, 4);
            offset += 4;

            Array.Copy(BitConverter.GetBytes((ushort)OverloadT), 0, buffer, offset, 2);
            offset += 2;
            Array.Copy(BitConverter.GetBytes((ushort)OverloadC), 0, buffer, offset, 2);

            var sb = new StringBuilder(buffer.Length * 2);
            foreach (var b in buffer)
            {
                sb.AppendFormat(CultureInfo.InvariantCulture, "{0:X2}", b);
            }

            hexString = sb.ToString();
            error = string.Empty;
            return true;
        }

        private static int ParseHexDigit(char c)
        {
            if (c >= '0' && c <= '9')
            {
                return c - '0';
            }

            if (c >= 'A' && c <= 'F')
            {
                return 10 + (c - 'A');
            }

            if (c >= 'a' && c <= 'f')
            {
                return 10 + (c - 'a');
            }

            throw new FormatException($"Carácter hex inválido: {c}");
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
