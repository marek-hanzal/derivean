import {redirect} from "next/navigation";

export namespace Index {
    export interface Props {
        params: {
            locale: string;
        };
    }
}

export default function Index({params: {locale}}: Index.Props) {
    redirect(`/${locale}/public`);
}
