import {IconLanguage}  from "@tabler/icons-react";
import {
    t,
    TranslationQueryStore,
    TranslationTable,
    tx
}                      from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {Page}          from "@use-pico/ui";

export default function List() {
    return <Page
        icon={<IconLanguage/>}
        text={{
            title:  tx()`Translation list (title)`,
            header: t()`Translation list`,
        }}
    >
        <StoreProvider
            store={TranslationQueryStore}
            values={{}}
        >
            <TranslationTable/>
        </StoreProvider>
    </Page>;
}
