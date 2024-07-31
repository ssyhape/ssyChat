const nodemailer = require('nodemailer');
const config_module = require("./config")


let transport = nodemailer.createTransport({
    host: 'smtp.163.com',
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user,
        pass: config_module.email_pass
    }
});

function SendMail(mailOptions_) {
    return new Promise()
}