import {
    Icon,
    Page
}          from "@use-pico/client";
import {t} from "@use-pico/translator";

export default function List() {
    return <Page
        icon={<Icon icon={"i-tabler-language"}/>}
        text={{
            header: t`Translation list`,
        }}
    >
        {/*<StoreProvider*/}
        {/*    store={TranslationQueryStore}*/}
        {/*    values={{}}*/}
        {/*>*/}
        {/*    <TranslationTable/>*/}
        {/*</StoreProvider>*/}
    </Page>;
}
