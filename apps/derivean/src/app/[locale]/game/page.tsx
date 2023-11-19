import {
    KingdomQueryProvider,
    KingdomTable
}                 from "@derivean/kingdom";
import {GameIcon} from "@derivean/ui";
import {
    Container,
    Page
}                 from "@use-pico/client";
import {t}        from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            header: t()`Game overview`,
        }}
    >
        <Container>
            <KingdomQueryProvider
                values={{}}
            >
                <KingdomTable
                    compact
                />
            </KingdomQueryProvider>
        </Container>
    </Page>;
}
