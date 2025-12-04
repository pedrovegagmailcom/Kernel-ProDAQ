using System;
using System.ComponentModel;
using System.Globalization;

namespace ProDAQConfig
{
    public class CellConfigModel : INotifyPropertyChanged
    {
        private string _serialNumber = string.Empty;
        private string _capacity = string.Empty;
        private string _limit = string.Empty;
        private string _resolution = string.Empty;
        private string _x1t = string.Empty;
        private string _x2t = string.Empty;
        private string _x3t = string.Empty;
        private string _x4t = string.Empty;
        private string _x1c = string.Empty;
        private string _x2c = string.Empty;
        private string _x3c = string.Empty;
        private string _x4c = string.Empty;
        private string _overloadT = string.Empty;
        private string _overloadC = string.Empty;

        public event PropertyChangedEventHandler PropertyChanged;

        public string SerialNumber
        {
            get => _serialNumber;
            set => SetField(ref _serialNumber, value ?? string.Empty, nameof(SerialNumber));
        }

        public string Capacity
        {
            get => _capacity;
            set => SetField(ref _capacity, value ?? string.Empty, nameof(Capacity));
        }

        public string Limit
        {
            get => _limit;
            set => SetField(ref _limit, value ?? string.Empty, nameof(Limit));
        }

        public string Resolution
        {
            get => _resolution;
            set => SetField(ref _resolution, value ?? string.Empty, nameof(Resolution));
        }

        public string X1T
        {
            get => _x1t;
            set => SetField(ref _x1t, value ?? string.Empty, nameof(X1T));
        }

        public string X2T
        {
            get => _x2t;
            set => SetField(ref _x2t, value ?? string.Empty, nameof(X2T));
        }

        public string X3T
        {
            get => _x3t;
            set => SetField(ref _x3t, value ?? string.Empty, nameof(X3T));
        }

        public string X4T
        {
            get => _x4t;
            set => SetField(ref _x4t, value ?? string.Empty, nameof(X4T));
        }

        public string X1C
        {
            get => _x1c;
            set => SetField(ref _x1c, value ?? string.Empty, nameof(X1C));
        }

        public string X2C
        {
            get => _x2c;
            set => SetField(ref _x2c, value ?? string.Empty, nameof(X2C));
        }

        public string X3C
        {
            get => _x3c;
            set => SetField(ref _x3c, value ?? string.Empty, nameof(X3C));
        }

        public string X4C
        {
            get => _x4c;
            set => SetField(ref _x4c, value ?? string.Empty, nameof(X4C));
        }

        public string OverloadT
        {
            get => _overloadT;
            set => SetField(ref _overloadT, value ?? string.Empty, nameof(OverloadT));
        }

        public string OverloadC
        {
            get => _overloadC;
            set => SetField(ref _overloadC, value ?? string.Empty, nameof(OverloadC));
        }

        public static CellConfigModel FromCsv(string csv)
        {
            if (string.IsNullOrWhiteSpace(csv))
            {
                throw new FormatException("La respuesta de configuración de célula está vacía.");
            }

            var parts = csv.Split(',');
            if (parts.Length != 14)
            {
                throw new FormatException("El formato de CellConfig no contiene los 14 campos requeridos.");
            }

            return new CellConfigModel
            {
                SerialNumber = parts[0],
                Capacity = parts[1],
                Limit = parts[2],
                Resolution = parts[3],
                X1T = parts[4],
                X2T = parts[5],
                X3T = parts[6],
                X4T = parts[7],
                X1C = parts[8],
                X2C = parts[9],
                X3C = parts[10],
                X4C = parts[11],
                OverloadT = parts[12],
                OverloadC = parts[13]
            };
        }

        public bool TryBuildPayload(out string payload, out string errorMessage)
        {
            payload = string.Empty;
            errorMessage = string.Empty;

            var serial = (SerialNumber ?? string.Empty).Trim();
            if (serial.Length == 0)
            {
                errorMessage = "Debe indicar el número de serie (máximo 10 caracteres).";
                return false;
            }

            if (serial.Length > 10)
            {
                serial = serial.Substring(0, 10);
            }

            if (!ushort.TryParse(Capacity, NumberStyles.Integer, CultureInfo.InvariantCulture, out var capacidad))
            {
                errorMessage = "Capacidad inválida (use enteros positivos).";
                return false;
            }

            if (!ushort.TryParse(Limit, NumberStyles.Integer, CultureInfo.InvariantCulture, out var limite))
            {
                errorMessage = "Límite inválido (use enteros positivos).";
                return false;
            }

            if (!TryParseFloat(Resolution, out var resolucion))
            {
                errorMessage = "Resolución inválida.";
                return false;
            }

            if (!TryParseFloat(X1T, out var x1t) ||
                !TryParseFloat(X2T, out var x2t) ||
                !TryParseFloat(X3T, out var x3t) ||
                !TryParseFloat(X4T, out var x4t) ||
                !TryParseFloat(X1C, out var x1c) ||
                !TryParseFloat(X2C, out var x2c) ||
                !TryParseFloat(X3C, out var x3c) ||
                !TryParseFloat(X4C, out var x4c))
            {
                errorMessage = "Alguno de los coeficientes no es válido.";
                return false;
            }

            if (!ushort.TryParse(OverloadT, NumberStyles.Integer, CultureInfo.InvariantCulture, out var overloadT))
            {
                errorMessage = "Sobrecarga tracción inválida (use enteros positivos).";
                return false;
            }

            if (!ushort.TryParse(OverloadC, NumberStyles.Integer, CultureInfo.InvariantCulture, out var overloadC))
            {
                errorMessage = "Sobrecarga compresión inválida (use enteros positivos).";
                return false;
            }

            payload = string.Join(",",
                serial,
                capacidad.ToString(CultureInfo.InvariantCulture),
                limite.ToString(CultureInfo.InvariantCulture),
                FormatFloat(resolucion),
                FormatFloat(x1t),
                FormatFloat(x2t),
                FormatFloat(x3t),
                FormatFloat(x4t),
                FormatFloat(x1c),
                FormatFloat(x2c),
                FormatFloat(x3c),
                FormatFloat(x4c),
                overloadT.ToString(CultureInfo.InvariantCulture),
                overloadC.ToString(CultureInfo.InvariantCulture));

            return true;
        }

        private static bool TryParseFloat(string value, out float result)
        {
            return float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out result);
        }

        private static string FormatFloat(float value)
        {
            return value.ToString("0.######", CultureInfo.InvariantCulture);
        }

        private void SetField<T>(ref T field, T value, string propertyName)
        {
            if (!Equals(field, value))
            {
                field = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
