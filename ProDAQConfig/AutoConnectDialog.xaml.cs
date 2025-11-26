using System;
using System.ComponentModel;
using System.Threading;
using System.Windows;

namespace ProDAQConfig
{
    public partial class AutoConnectDialog : Window, INotifyPropertyChanged
    {
        private readonly CancellationTokenSource _cancellationTokenSource;
        private string _progressMessage = "Preparando búsqueda...";

        public AutoConnectDialog(CancellationTokenSource cancellationTokenSource)
        {
            _cancellationTokenSource = cancellationTokenSource ?? throw new ArgumentNullException(nameof(cancellationTokenSource));
            InitializeComponent();
            DataContext = this;
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public string ProgressMessage
        {
            get => _progressMessage;
            set
            {
                if (_progressMessage != value)
                {
                    _progressMessage = value;
                    OnPropertyChanged(nameof(ProgressMessage));
                }
            }
        }

        private void CancelButton_OnClick(object sender, RoutedEventArgs e)
        {
            _cancellationTokenSource.Cancel();
            ProgressMessage = "Búsqueda cancelada por el usuario.";
            Close();
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
