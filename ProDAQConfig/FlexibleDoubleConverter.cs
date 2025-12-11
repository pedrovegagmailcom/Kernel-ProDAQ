using System;
using System.Globalization;
using System.Windows.Data;

namespace ProDAQConfig
{
    public class FlexibleDoubleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value == null)
            {
                return string.Empty;
            }

            if (value is double doubleValue)
            {
                var formatCulture = culture ?? CultureInfo.CurrentCulture;
                return doubleValue.ToString(formatCulture);
            }

            return value.ToString();
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            var text = value?.ToString() ?? string.Empty;

            if (string.IsNullOrWhiteSpace(text))
            {
                return 0d;
            }

            var parseCulture = culture ?? CultureInfo.CurrentCulture;
            var decimalSeparator = parseCulture.NumberFormat.NumberDecimalSeparator;
            var alternateSeparator = decimalSeparator == "," ? "." : ",";
            var groupSeparator = parseCulture.NumberFormat.NumberGroupSeparator;

            var normalized = text.Trim();
            if (!string.IsNullOrEmpty(groupSeparator))
            {
                normalized = normalized.Replace(groupSeparator, string.Empty);
            }

            normalized = normalized.Replace(alternateSeparator, decimalSeparator);

            if (double.TryParse(normalized, NumberStyles.Float, parseCulture, out var cultureValue))
            {
                return cultureValue;
            }

            normalized = normalized.Replace(decimalSeparator, ".");

            if (double.TryParse(normalized, NumberStyles.Float, CultureInfo.InvariantCulture, out var invariantValue))
            {
                return invariantValue;
            }

            return Binding.DoNothing;
        }
    }
}
