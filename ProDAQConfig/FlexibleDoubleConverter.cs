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
                return doubleValue.ToString(CultureInfo.InvariantCulture);
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

            if (double.TryParse(text, NumberStyles.Float, CultureInfo.CurrentCulture, out var currentCultureValue))
            {
                return currentCultureValue;
            }

            text = text.Replace(',', '.');

            if (double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out var invariantValue))
            {
                return invariantValue;
            }

            return Binding.DoNothing;
        }
    }
}
