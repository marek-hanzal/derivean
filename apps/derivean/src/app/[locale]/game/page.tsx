import {
    KingdomQueryStore,
    KingdomTable
}                      from "@derivean/kingdom";
import {GameIcon}      from "@derivean/ui";
import {t}             from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Container,
    Page
}                      from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            header: t()`Game overview`,
        }}
    >
        <Container>
            <StoreProvider
                store={KingdomQueryStore}
                values={{}}
            >
                <KingdomTable
                    compact
                />
            </StoreProvider>
        </Container>
    </Page>;
}
