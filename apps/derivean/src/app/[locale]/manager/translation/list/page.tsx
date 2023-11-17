import {IconLanguage} from "@tabler/icons-react";
import {Page}         from "@use-pico/client";
import {t}            from "@use-pico/translator";

export default function List() {
    return <Page
        icon={<IconLanguage/>}
        text={{
            header: t()`Translation list`,
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
