Imports System.Windows.Forms
Imports System.Runtime.InteropServices
Imports ASCOM.Utilities
Imports ASCOM.GuidePortBuddy
Imports System.IO.Ports
Imports System.Threading

<ComVisible(False)>
Public Class SetupDialogForm

    Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click ' OK button event handler
        ' Persist new values of user settings to the ASCOM profile
        Telescope.comPort = ComboBoxComPort.SelectedItem ' Update the state variables with results from the dialogue
        Telescope.traceState = chkTrace.Checked
        Me.DialogResult = System.Windows.Forms.DialogResult.OK
        Me.Close()
    End Sub

    Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click 'Cancel button event handler
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub

    Private Sub ShowAscomWebPage(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles PictureBox1.DoubleClick, PictureBox1.Click
        ' Click on ASCOM logo event handler
        Try
            System.Diagnostics.Process.Start("http://ascom-standards.org/")
        Catch noBrowser As System.ComponentModel.Win32Exception
            If noBrowser.ErrorCode = -2147467259 Then
                MessageBox.Show(noBrowser.Message)
            End If
        Catch other As System.Exception
            MessageBox.Show(other.Message)
        End Try
    End Sub

    Private Sub SetupDialogForm_Load(sender As System.Object, e As System.EventArgs) Handles MyBase.Load ' Form load event handler
        ' Retrieve current values of user settings from the ASCOM Profile
        InitUI()
    End Sub

    Private Sub InitUI()
        chkTrace.Checked = Telescope.traceState
        ' set the list of com ports to those that are currently available
        ComboBoxComPort.Items.Clear()
        ComboBoxComPort.Items.AddRange(System.IO.Ports.SerialPort.GetPortNames())       ' use System.IO because it's static
        ' select the current port if possible
        If ComboBoxComPort.Items.Contains(Telescope.comPort) Then
            ComboBoxComPort.SelectedItem = Telescope.comPort
        End If
    End Sub
    Private Sub btnReset_Click(sender As Object, e As EventArgs) Handles btnReset.Click
        serialCommand("R")
        MsgBox("GuidePortBuddy has been reset",, "GuidePortBuddy")
    End Sub
    Private Sub btnCheck_Click(sender As Object, e As EventArgs) Handles btnCheck.Click
        lstDetails.Items.Clear()
        lstDetails.Visible = True
        If serialCommand("A") = "A" Then
            lstDetails.Items.Add("GuidePortBuddy responding")
            lstDetails.Items.Add(serialCommand("V"))
            lstDetails.Items.Add(serialCommand("L"))
        Else
            lstDetails.Items.Add("GuidePortBuddy not responding - check connection")
        End If
    End Sub
    Private Function serialCommand(cmdStr As String) As String
        Dim retStr As String = ""
        Dim objSerial As New SerialPort
        Try
            With objSerial
                .PortName = ComboBoxComPort.SelectedItem.ToString
                .BaudRate = 115200
                .ReadTimeout = 5000
                .WriteTimeout = 5000
                .Open()
                .Write(cmdStr)
            End With
            retStr = objSerial.ReadTo("#")
        Catch ex As Exception
            MsgBox("Can't open " + objSerial.PortName, MsgBoxStyle.Critical, "GuidePortBuddy")
            retStr = "Error"
        End Try

        Try
            objSerial.Close()
            objSerial.Dispose()
        Catch ex As Exception
        End Try

        Return retStr
    End Function

End Class
