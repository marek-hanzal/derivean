import {getToken} from "@use-pico/auth-server";
import {redirect} from "next/navigation";

export namespace Index {
    export interface Props {
        params: {
            locale: string;
        };
    }
}

export default async function Index({params: {locale}}: Index.Props) {
    redirect(await getToken() ? `/${locale}/game` : `/${locale}/public`);
}
