import "@unocss/reset/tailwind.css";
import {type Metadata}          from "next";
import {type PropsWithChildren} from "react";
import "uno.css";

export const metadata: Metadata = {
    title: "The Legacy of DeRivean",
};

export default function Layout({children}: PropsWithChildren) {
    return <html lang={"en"}>
        <body>
            {children}
        </body>
    </html>;
}
